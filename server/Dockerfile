# Building the binary of the App
FROM golang:1.24.2 AS build

# setting the workdir
WORKDIR /go/src/build

# Copy all the Code and stuff to compile everything
COPY . .

# Downloads all the dependencies in advance (could be left out, but it's more clear this way)
RUN go mod download

ARG TARGETARCH
# Builds the application as a staticly linked one, to allow it to run on alpine
RUN CGO_ENABLED=0 GOOS=linux GOARCH=${TARGETARCH} go build -a -installsuffix cgo -o app .


# Moving the binary to the 'final Image' to make it smaller
FROM alpine:latest as release

WORKDIR /app

# Create the `public` dir and copy all the assets into it
# RUN mkdir ./views
# COPY ./views ./views
# COPY ./static ./static

#  should be replaced here as well
COPY --from=build /go/src/build/app .

# Add packages
RUN apk -U upgrade \
    && apk add --no-cache dumb-init ca-certificates \
    && chmod +x /app/app

# Exposes port 3000 because our program listens on that port
EXPOSE 3000

ENTRYPOINT ["/usr/bin/dumb-init", "/app/app"]