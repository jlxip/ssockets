FROM alpine:3.17 as build

RUN apk add --no-cache git make gcc musl-dev

RUN git clone https://github.com/jlxip/ssockets ~/ssockets
RUN sed -i 's/\/bin\/bash/\/bin\/sh/g' ~/ssockets/Makefile
RUN make -C ~/ssockets install

# Cleanup
RUN rm -rf ~/ssockets
RUN apk del git make gcc musl-dev

# Flatten time
FROM scratch
COPY --from=build / /
