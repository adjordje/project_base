FROM alpine:edge

RUN apk update

RUN apk add \
    build-base \
    libx11-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxi-dev \
    mesa-dev

RUN apk add mesa-dri-gallium xvfb-run

RUN apk add virtualgl --update-cache --repository http://dl-3.alpinelinux.org/alpine/edge/testing/ --allow-untrusted

WORKDIR /app

COPY cobraShaderExpo .

ENTRYPOINT ["./cobraShaderExpo"]
