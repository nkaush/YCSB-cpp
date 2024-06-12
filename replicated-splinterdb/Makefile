REPO       := neilk3
YCSB_IMAGE := $(REPO)/ycsb-replicated-splinterdb
SERVER_IMAGE := $(REPO)/replicated-splinterdb

all: $(YCSB_IMAGE) $(SERVER_IMAGE)

$(YCSB_IMAGE):
	$(MAKE) -C replicated-splinterdb \
		neilk3/splinterdb-build-env \
		neilk3/splinterdb-run-env 
	docker build -t $@ -f Dockerfile ..

$(SERVER_IMAGE):
	$(MAKE) -C replicated-splinterdb neilk3/replicated-splinterdb
