include scripts.mk

prefix=/usr
export DEBUG=y

subdir-y+=src/

subdir-$(TEST)+=test/
