# Scalable and Secure Platform for Genomic Computing

## Architecture & Documentation

[Architecture](https://docs.google.com/document/d/1IDGdtbbdGEsr3EHXDMzaGw8ffRHLqY2JT5hw5VVLdHs/edit?usp=sharing)

## Prerequsites

1. ZeroMQ

```bash
sudo apt-get install libzmq3-dev
```

## Messages

### Worker/Client to Driver

#### Base format

<COMMAND> <SOURCE> <PARAMETERS>

#### Join

JIN <worker_id/client_id>

#### Message

MSG <worker_id/client_id> <payload>

#### Acknowledgement

ACK <worker_id/client_id>

### Driver to Worker/Client

#### Acknowledgement

ACK

### Message

MSG <payload>
