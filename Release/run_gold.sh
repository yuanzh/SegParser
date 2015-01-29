args=$1
runid=$2

ln -s $args.seg.gold.train ../../data/$args/$args.train.$runid
ln -s $args.seg.gold.test ../../data/$args/$args.test.$runid

./SegParser train train-file:../../data/$args/$args.train.$runid model-name:../../data/$args/$args.model.$runid decode-type:non-proj test test-file:../../data/$args/$args.test.$runid seed:${runid} $@

    rm ../../data/$args/$args.train.$runid
    rm ../../data/$args/$args.test.$runid

