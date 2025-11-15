# pflib Development Environment
If you're like me and you want your development environment to be
identical to the production environment (the ZCU in this case),
I've written this `Containerfile` that can build an image replicating
the ZCU and Bittware host _software_ environment.

The image will be missing the _firmware_ environment necessary to do
all of the communication tasks, but it can still be helpful to develop
on a friendlier system with more cores.

## ZCU
- GCC 11.4.0
- Git 2.47.1
- Boost 1.75.0
- nlohmann_json 3.10.5
- yaml-cpp 0.6.3
  - The image here uses 0.8.0 and I'm too lazy to fix it

## Bittware Host
- GCC 11.4.0
- Git 2.34.1
- Boost 1.74.0
- nlohmann_json 3.10.5
- yaml-cpp 0.7.0

### Build Image
You can certainly build the image yourself, but there is a pre-built image
hosted on the GitHub Container Registry (ghcr) which is what is used in
the justfile and built with [a GitHub workflow](../.github/workflows/build-test-image.yml).
```
cd env
# what I did
podman image build . -f Containerfile.<host> -t pflib-env:<host>-latest
# I think this can work
docker build . -f env/Containerfile.<host> -t pflib-env:<host>-latest
```

### Initialize
The image name after `init` is what defines which image you want to use.
Below, I'm using the same name as the one I used above when building
the image locally. The `justfile` uses the image built and stored on GitHub.
```
denv init pflib-env:<host>-latest
```
