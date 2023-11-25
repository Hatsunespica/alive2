search_dir=./tests

for entry in "$search_dir"/*
do
  echo "$entry"
  alive-mutate $entry ./tmp -n 10 --disable-undef-input --disable-poison-input --src-unroll 2 --tgt-unroll 3 --saveAll
done
