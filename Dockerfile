# Test building on Alpine Linux... better than not testing compilation on any Linux at all.

FROM alpine:3.7
COPY . /app
RUN  apk update && \
		 apk upgrade && \
		 apk add build-base && \
		 cd /app && \
		 make clean all
CMD  ./tree -sf /app

# docker build -t tree .
# docker system prune
