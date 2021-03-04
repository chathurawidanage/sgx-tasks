# Building docker

docker build . --no-cache
docker tag <image_id> cwidanage/tasker:<version>
docker push cwidanage/tasker:<version>