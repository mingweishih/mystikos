FROM mcr.microsoft.com/dotnet-buildtools/prereqs:ubuntu-16.04-20210924170333-538077f as builder

# Step 1. Build .Net runtime + library tests
WORKDIR /build
RUN git clone --depth 1 --single-branch --branch v5.0.11 https://github.com/dotnet/runtime.git
WORKDIR /build/runtime/

# Build clr+libs+libs.tests in Release
RUN ./build.sh -subset clr+libs+libs.tests \
    -c Release -rc Release -lc Release 

# To run all library tests
# RUN ./build.sh -subset libs.tests -test -testnobuild \
#     -c Release -rc Release -lc Release

# Step 2. Build SDK
WORKDIR /build
RUN git clone --depth 1 --single-branch --branch v5.0.401 https://github.com/dotnet/sdk.git
WORKDIR /build/sdk/
# Override CLI starttime check
COPY ./Program.cs ./src/Cli/dotnet/
RUN ./build.sh -c Release -p:Version=5.0.401

# Step 3. Extract built artifacts
# https://hub.docker.com/_/microsoft-dotnet-runtime/
FROM mcr.microsoft.com/dotnet/runtime:5.0.11-focal-amd64

WORKDIR /dotnet-lib-release
# We need correct version of SDK + runtime to run a test
# We will use the dotnet binary + runtime under ./artifacts/bin/testhost/net5.0-Linux-Release-x64/
COPY --from=builder /build/runtime/artifacts/bin/ ./

# Grab modified dotnet SDK
COPY --from=builder /build/sdk/artifacts/bin/redist/Release/dotnet/sdk/5.0.401/ /dotnet-lib-release/testhost/net5.0-Linux-Release-x64/sdk/5.0.401
