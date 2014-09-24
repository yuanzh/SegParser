args=$1
runid=$2

ln -s $args.seg.test ../../data/$args/$args.test.$runid

./SegParser model-name:../../data/$args/$args.model.$runid decode-type:non-proj test test-file:../../data/$args/$args.test.$runid seed:${runid} $@

    rm ../../data/$args/$args.test.$runid

