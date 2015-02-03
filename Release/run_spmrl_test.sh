runid=$1

ln -s spmrl.seg.test ../data/spmrl.test.$runid

./SegParser model-name:../runs/spmrl.model.$runid test test-file:../data/spmrl.test.$runid output-file:../runs/spmrl.out.$runid seed:2 evalpunc:true test-converge:200 tedeval:true devthread:10 $@

    rm ../data/spmrl.test.$runid

