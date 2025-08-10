
docker run --init \
	   -ti \
	   -p 80:80 \
	   --cap-add=SYS_ADMIN \
	   --security-opt seccomp=unconfined \
	   --security-opt apparmor=unconfined \
	   potato2 $*

