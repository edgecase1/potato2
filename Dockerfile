FROM gcc AS builder

RUN apt-get update -y && apt-get install libssl-dev 
WORKDIR /build
COPY ./src/ .
RUN make
RUN ls

FROM debian:12-slim
RUN apt-get update -y && apt-get install -y libssl3
RUN groupadd -g 22222 potato
WORKDIR /app
COPY --from=builder /build/potato .
COPY userlist .

ENTRYPOINT ["./potato"]
