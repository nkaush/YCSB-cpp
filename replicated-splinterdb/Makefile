REPO       := neilk3
YCSB_IMAGE := $(REPO)/ycsb-replicated-splinterdb

$(YCSB_IMAGE):
	$(MAKE) -C replicated-splinterdb neilk3/replicated-splinterdb
	docker build -t $@ -f Dockerfile ..
