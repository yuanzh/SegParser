runid=$1

ln -s spmrl.seg.train ../../data/spmrl/spmrl.train.$runid
ln -s spmrl.seg.dev ../../data/spmrl/spmrl.test.$runid

./SegParser train train-file:../../data/spmrl/spmrl.train.$runid model-name:../../data/spmrl/spmrl.model.$runid decode-type:proj test-file:../../data/spmrl/spmrl.test.$runid seed:${runid} $@

    rm ../../data/spmrl/spmrl.train.$runid
    rm ../../data/spmrl/spmrl.test.$runid

