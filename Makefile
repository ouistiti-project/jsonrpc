include scripts.mk

prefix=/usr
export JSONSOCKET=y
export DEBUG=y

subdir-y+=src/
subdir-$(TEST)+=test/
