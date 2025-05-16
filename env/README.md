# pflib Development Environment
If you're like me and you want your development environment to be
identical to the production environment (the ZCU in this case),
I've written this `Containerfile` that can build an image replicating
the ZCU _software_ environment.

The image will be missing the _firmware_ environment necessary to do
all of the communication tasks, but it can still be helpful to develop
on a friendlier system with more cores.

### Build Image
```
# what I did
podman image build env/ -t pflib-env:latest
# I think this can work
docker build env/ -f env/Containerfile -t pflib-env:latest
```

### Initialize
```
denv init pflib-env:latest
```
