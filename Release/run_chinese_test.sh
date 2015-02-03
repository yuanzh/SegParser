runid=$1

ln -s ctb.seg.test ../data/ctb.test.$runid

./SegParser model-name:../runs/ctb.model.$runid test test-file:../data/ctb.test.$runid output-file:../runs/ctb.out.$runid seed:14 evalpunc:false test-converge:300 $@

    rm ../data/ctb.test.$runid

