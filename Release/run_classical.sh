runid=$1

ln -s qatar.seg.train ../data/qatar.train.$runid
ln -s qatar.seg.test ../data/qatar.test.$runid

./SegParser train test train-file:../data/qatar.train.$runid model-name:../runs/qatar.model.$runid test-file:../data/qatar.test.$runid output-file:../runs/qatar.out.$runid seed:1 ho:false earlystop:20 evalpunc:true C:0.0001 train-converge:200 test-converge:200 savebest:false iters:5 $@ | tee ../runs/qatar.log.$runid

    rm ../data/qatar.train.$runid
    rm ../data/qatar.test.$runid

