;pr4917_4
;current seed:629901056
; ModuleID = './tests/test2.ll'
source_filename = "./tests/test2.ll"

define i32 @pr4917_4(i32 %x, i32 %y, i32 %0) {
entry:
  %l = zext i32 %x to i64
  %r = zext i32 %0 to i64
  %mul64 = mul i64 %l, %r
  %overflow = icmp ule i64 %mul64, 4294967295
  %retval = zext i1 %overflow to i32
  ret i32 %retval
}
