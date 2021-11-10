#include "ComplexMutator.h"

bool ComplexMutator::init(){
    for(auto funcIt=pm->begin();funcIt!=pm->end();++funcIt){
        for(auto ait=funcIt->arg_begin();ait!=funcIt->arg_end();++ait){
            if(ait->hasAttribute(llvm::Attribute::AttrKind::ImmArg)){
                filterSet.insert(funcIt->getName().str());
                break;
            }
        }
        if(!funcIt->isDeclaration()&&!funcIt->getName().empty()){
            dtMap[funcIt->getName()]=llvm::DominatorTree(*funcIt);
        }
    }

    bool result=false;
    for(fit=pm->begin();fit!=pm->end();++fit){
        if(fit->isDeclaration()){
            continue;
        }
        for(bit=fit->begin();bit!=fit->end();++bit)
            for(iit=bit->begin();iit!=bit->end();++iit){
                if(isReplaceable(&*iit)){
                    result=true;
                    goto end;

                }
            }
    }
end:
    if(result){
        for(auto git=pm->global_begin();git!=pm->global_end();++git){
            domInst.push_back(&*git);
        }
        calcDomInst();
    }

    return result;
}

void ComplexMutator::resetTmpModule(){
    vMap.clear();
    tmpCopy=llvm::CloneModule(*pm,vMap);
    tmpFit=llvm::Module::iterator((llvm::Function*)&*vMap[&*fit]);
    tmpBit=llvm::Function::iterator((llvm::BasicBlock*)&*vMap[&*bit]);
    tmpIit=llvm::BasicBlock::iterator((llvm::Instruction*)&*vMap[&*iit]);
}

void ComplexMutator::mutateModule(const std::string& outputFileName){
    resetTmpModule();    
    if(debug){
        llvm::errs()<<"Current function "<<tmpFit->getName()<<"\n";
        llvm::errs()<<"Current basic block:\n";
        tmpBit->print(llvm::errs());
        llvm::errs()<<"\nCurrent instruction:\n";
        tmpIit->print(llvm::errs());
        llvm::errs()<<"\n";

    }
    currFuncName=tmpFit->getName().str();
    bool newAdded=false;
    //75% chances to add a new inst, 25% chances to replace with a existent usage
    if((Random::getRandomUnsigned()&3)!=0){
        insertRandomBinaryInstruction(&*tmpIit);
        newAdded=true;
    }else{
        replaceRandomUsage(&*tmpIit);
    }
    if(debug){
        if(!newAdded){
            llvm::errs()<<"\nReplaced with a existant usage\n";
        }else{
            llvm::errs()<<"\nNew Inst added\n";
        }
        tmpBit->print(llvm::errs());
        llvm::errs()<<"\nDT info"<<dtMap.find(fit->getName())->second.dominates(&*(fit->getFunction().begin()->begin()),&*iit);
        llvm::errs()<<"\n";
        /*std::error_code ec;
        llvm::raw_fd_ostream fout(outputFileName,ec);
        fout<<*tmpCopy;
        fout.close();
        llvm::errs()<<"file wrote to "<<outputFileName<<"\n";*/
    }
    moveToNextReplaceableInst();
}

void ComplexMutator::saveModule(const std::string& outputFileName){
    std::error_code ec;
    llvm::raw_fd_ostream fout(outputFileName,ec);
    fout<<*tmpCopy;
    fout.close();
    llvm::errs()<<"file wrote to "<<outputFileName<<"\n";
}

bool ComplexMutator::isReplaceable(llvm::Instruction* inst){
    //contain immarg attributes
    if(llvm::isa<llvm::CallBase>(inst)){
        //in case of cannot find function name
        if(llvm::Function* func=((llvm::CallBase*)inst)->getCalledFunction();func!=nullptr&&
            (filterSet.find(func->getName().str())!=filterSet.end()||func->getName().startswith("llvm"))){
            return false;
        }
    }
    //don't do replacement on PHI node
    //don't update an alloca inst
    //don't do operations on Switch inst for now.
    if(llvm::isa<llvm::PHINode>(inst)||llvm::isa<llvm::GetElementPtrInst>(inst)||llvm::isa<llvm::AllocaInst>(inst)
        ||llvm::isa<llvm::SwitchInst>(inst)){

        return false;
    }

    for(llvm::Use& u:inst->operands()){
        if(u.get()->getType()->isIntegerTy()){
            return true;
        }
    }
    return false;
}

void ComplexMutator::moveToNextFuction(){
    ++fit;
    if(fit==pm->end())fit=pm->begin();
    while(fit->isDeclaration()){
        ++fit;if(fit==pm->end())fit=pm->begin();
    }
    bit=fit->begin();
    iit=bit->begin();
}

void ComplexMutator::moveToNextBasicBlock(){
    ++bit;
    if(bit==fit->end()){
        moveToNextFuction();
    }else{
        iit=bit->begin();
    }
    calcDomInst();
}

void ComplexMutator::moveToNextInst(){
    domInst.push_back(&*iit);
    ++iit;
    if(iit==bit->end()){
        moveToNextBasicBlock();
    }
}

void ComplexMutator::moveToNextReplaceableInst(){
    moveToNextInst();
    while(!isReplaceable(&*iit))moveToNextInst();
}


void ComplexMutator::calcDomInst(){
    domInst.resize(pm->global_size());
    if(auto it=dtMap.find(fit->getName());it!=dtMap.end()){
        //add Parameters
        for(auto ait=fit->arg_begin();ait!=fit->arg_end();++ait){
            domInst.push_back(&*ait);
        }
        llvm::DominatorTree& DT=it->second;
        //add BasicBlocks before bitTmp
        for(auto bitTmp=fit->begin();bitTmp!=bit;++bitTmp){
            if(DT.dominates(&*bitTmp,&*bit)){
                for(auto iitTmp=bitTmp->begin();iitTmp!=bitTmp->end();++iitTmp){
                        domInst.push_back(&*iitTmp);
                }
            }
        }
        //add Instructions before iitTmp
        for(auto iitTmp=bit->begin();iitTmp!=iit;++iitTmp){
            if(DT.dominates(&*iitTmp,&*iit)){
                domInst.push_back(&*iitTmp);
            }
        }
    }
}

void ComplexMutator::replaceRandomUsage(llvm::Instruction* inst){
    size_t pos=Random::getRandomUnsigned()%inst->getNumOperands();;
    llvm::Type* ty=nullptr;
    for(size_t i=0;i<inst->getNumOperands();++i,++pos){
        if(pos==inst->getNumOperands())pos=0;
        if(inst->getOperand(pos)->getType()->isIntegerTy()){
            ty=inst->getOperand(pos)->getType();
            break;
        }
    }
    llvm::Value* val=getRandomValue(ty);
    inst->setOperand(pos,val);
}

void ComplexMutator::insertRandomBinaryInstruction(llvm::Instruction* inst){
    size_t pos=Random::getRandomUnsigned()%inst->getNumOperands();
    llvm::Type* ty=nullptr;
    for(size_t i=0;i<inst->getNumOperands();++i,++pos){
        if(pos==inst->getNumOperands())pos=0;
        if(inst->getOperand(pos)->getType()->isIntegerTy()){
            ty=inst->getOperand(pos)->getType();
            break;
        }
    }
    
    llvm::Value* val1=getRandomValue(ty),*val2=getRandomValue(ty);
    llvm::Instruction::BinaryOps Op;

    using llvm::Instruction;
    switch (Random::getRandomUnsigned() % 13) {
        default: llvm_unreachable("Invalid BinOp");
        case 0:{Op = Instruction::Add; break; }
        case 1:{Op = Instruction::Sub; break; }
        case 2:{Op = Instruction::Mul; break; }
        case 3:{Op = Instruction::SDiv; break; }
        case 4:{Op = Instruction::UDiv; break; }
        case 5:{Op = Instruction::SRem; break; }
        case 6:{Op = Instruction::URem; break; }
        case 7: {Op = Instruction::Shl;  break; }
        case 8: {Op = Instruction::LShr; break; }
        case 9: {Op = Instruction::AShr; break; }
        case 10:{Op = Instruction::And;  break; }
        case 11:{Op = Instruction::Or;   break; }
        case 12:{Op = Instruction::Xor;  break; }
    }

    llvm::Instruction* newInst=llvm::BinaryOperator::Create(Op, val1, val2, "", inst);
    inst->setOperand(pos,newInst);
}

llvm::Constant* ComplexMutator::getRandomConstant(llvm::Type* ty){
    if(ty->isIntegerTy()){
        return llvm::ConstantInt::get(ty,Random::getRandomUnsigned());
    }
    return llvm::UndefValue::get(ty);
}

llvm::Value* ComplexMutator::getRandomValue(llvm::Type* ty){
    if(ty!=nullptr&&!domInst.empty()){
        for(size_t i=0,pos=Random::getRandomUnsigned()%domInst.size();i<domInst.size();++i,++pos){
            if(pos==domInst.size())pos=0;
            if(domInst[pos]->getType()==ty){
                return &*vMap[domInst[pos]];
            }
        }
    }
    return getRandomConstant(ty);
}