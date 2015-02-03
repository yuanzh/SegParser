runid=$1

ln -s ctb.seg.train ../data/ctb.train.$runid
ln -s ctb.seg.dev ../data/ctb.test.$runid

./SegParser train test train-file:../data/ctb.train.$runid model-name:../runs/ctb.model.$runid test-file:../data/ctb.test.$runid output-file:../runs/ctb.out.$runid seed:14 earlystop:40 evalpunc:false C:0.01 train-converge:300 test-converge:300 $@ | tee ../runs/ctb.log.$runid

    rm ../data/ctb.train.$runid
    rm ../data/ctb.test.$runid

