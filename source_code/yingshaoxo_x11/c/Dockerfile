FROM alpine:3.18

# Install minimal build dependencies for X11
RUN apk add --no-cache \
    alpine-sdk \
    libx11-dev \
    libx11-static \
    libxcb-dev \
    libxcb-static \
    libxau-dev \
    libxdmcp-dev \
    linux-headers

WORKDIR /app

# Create a non-root user
RUN adduser -D builder
RUN chown -R builder:builder /app

# Switch to the non-root user
USER builder