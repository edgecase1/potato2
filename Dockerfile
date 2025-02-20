FROM gcc:14 AS builder

RUN apt-get update -y && apt-get install libssl-dev 
WORKDIR /build
COPY ./src/ .
RUN make
RUN ls

FROM ubuntu:24.04
RUN apt-get update -y && apt-get install -y libssl3 libc6 
RUN groupadd -g 22222 potato
WORKDIR /app
COPY --from=builder /build/potato .
COPY userlist .

EXPOSE 222
ENTRYPOINT ["./potato"]
