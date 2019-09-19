FROM debian:stretch
LABEL maintainer="Beenode Developers <dev@beenode.org>"
LABEL description="Dockerised BeenodeCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
