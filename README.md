#### SegParser

Randomized Greedy algorithm for joint segmentation, POS tagging and dependency parsing

=========

#### Usage

##### 1. Compilation

To compile the project, first make sure you have installed boost and boost-regex on your machine. Next, go to the "Release" directory and run command "make all" to compile the code. Note that the implementation uses some c++0x/c++11 features. Please make sure your compiler supports them.

<br> 

##### 2. Data Format

The data format for each sentence has two parts. The first part is similar to the one used in CoNLL-X shared task. The only difference is the index in the first column. Here the index format is "token index/segment index", where the token index starts from 1 (0 is for the root), while the segment index starts from 0.

The second part encodes the search space for segmentation and POS tagging. Each line contains a string for the lattice structure of each token. The format is as follows.

line := Token form\tCandidate1\tCandidate2\t...

Candidate := Segmentation||Al index||Morphology index||Morphology value||Candidate probability

Segmentation := Segment1&&Segment2&&...

Segment := Surface form@#Lemma form@#POS candidate1@#POS candidate2@#...

POS candidate := POS tag_probability

"data" directory includes sample data files for the SPMRL dataset.

##### 3. Datasets

Because of the license issue, datasets are not directly released here. You can find sample files in "data" directory. Please contact me for the full dataset if you are interested in.

UPDATE: data generator for SPMRL dataset and needed files for generating testing data are added into the directory spmrl_data_generator.

##### 4. Usage

Take a look at the scripts "run_DATA.sh" and "run_DATA_test.sh" where DATA=spmrl|classical|chinese. For example, to train a model on the SPMRL dataset, you can simply run

run_spmrl.sh run1

The model and development results will be saved in directory "runs". Note that the model is evaluated on the development set (if exists) after each epoch *in parallel* with the training. After the model is trained, you can evaluate it on the test set by running

run_spmrl_test.sh run1

