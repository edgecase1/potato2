
docker run --name potato \
	   --rm \
           --init \
	   -ti \
	   -p 80:80 \
	   --network=host \
	   --pid=host \
	   --cap-add=SYS_ADMIN \
	   --security-opt seccomp=unconfined \
	   --security-opt apparmor=unconfined \
	   potato2 $*

