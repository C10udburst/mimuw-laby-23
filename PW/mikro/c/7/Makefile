pipeline: err.c pipeline-utils.c pipeline.c

pipeline-tester: err.c pipeline-utils.c pipeline-tester.c

test: pipeline-tester
	./pipeline-tester

pipe1: ./pipeline
	./pipeline ls

pipe2: ./pipeline
	./pipeline ls wc

pipe3: ./pipeline
	./pipeline ls cat wc