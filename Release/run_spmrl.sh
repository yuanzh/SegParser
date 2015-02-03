runid=$1

ln -s spmrl.seg.train ../data/spmrl.train.$runid
ln -s spmrl.seg.dev ../data/spmrl.test.$runid

./SegParser train test train-file:../data/spmrl.train.$runid model-name:../runs/spmrl.model.$runid test-file:../data/spmrl.test.$runid output-file:../runs/spmrl.out.$runid seed:2 earlystop:20 evalpunc:true C:0.01 train-converge:200 test-converge:200 $@ | tee ../runs/spmrl.log.$runid

    rm ../data/spmrl.train.$runid
    rm ../data/spmrl.test.$runid

