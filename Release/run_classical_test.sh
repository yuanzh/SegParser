runid=$1

ln -s qatar.seg.test ../data/qatar.test.$runid

./SegParser model-name:../runs/qatar.model.$runid test test-file:../data/qatar.test.$runid output-file:../runs/qatar.out.$runid seed:1 ho:false evalpunc:true test-converge:200 devthread:10 $@

    rm ../data/qatar.test.$runid

