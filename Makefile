CONTIKI_PROJECT = coap-example-server
all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

# Include the CoAP implementation
MODULES += os/net/app-layer/coap
MODULES += os/net/ipv6/multicast

# Include CoAP resources
MODULES_REL += ./resources

#Activate DTLS
MAKE_WITH_DTLS=1
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
MAKE_COAP_DTLS_KEYSTORE = MAKE_COAP_DTLS_KEYSTORE_SIMPLE

CONTIKI=../..
include $(CONTIKI)/Makefile.include
