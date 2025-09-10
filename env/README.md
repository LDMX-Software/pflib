# pflib Development Environment
If you're like me and you want your development environment to be
identical to the production environment (the ZCU in this case),
I've written this `Containerfile` that can build an image replicating
the ZCU _software_ environment.

The image will be missing the _firmware_ environment necessary to do
all of the communication tasks, but it can still be helpful to develop
on a friendlier system with more cores.

### Build Image
You can certainly build the image yourself, but there is a pre-built image
hosted on the GitHub Container Registry (ghcr) which is what is used in
the justfile and built with [a GitHub workflow](../.github/workflows/build-test-image.yml).
```
# what I did
podman image build env/ -t pflib-env:latest
# I think this can work
docker build env/ -f env/Containerfile -t pflib-env:latest
```

### Initialize
The image name after `init` is what defines which image you want to use.
Below, I'm using the same name as the one I used above when building
the image locally. The `justfile` uses the image built and stored on GitHub.
```
denv init pflib-env:latest
```
