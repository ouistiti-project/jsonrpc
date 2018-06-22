include scripts.mk

prefix=/usr
export JSONSOCKET=y
export DEBUG=y

TEST?=y
export TEST
TESTSERVICE?=zmq
export TESTSERVICE

subdir-y+=src/
subdir-$(TEST)+=test/
