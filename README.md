# Caffe2

[![License](https://img.shields.io/badge/License-Apache%202.0-brightgreen.svg)](https://opensource.org/licenses/Apache-2.0)
[![Jenkins Build Status](https://ci.pytorch.org/jenkins/job/caffe2-master/badge/icon)](https://ci.pytorch.org/jenkins/job/caffe2-master)
[![Appveyor Build Status](https://img.shields.io/appveyor/ci/Yangqing/caffe2.svg)](https://ci.appveyor.com/project/Yangqing/caffe2)

Caffe2 is a lightweight, modular, and scalable deep learning framework. Building on the original [Caffe](http://caffe.berkeleyvision.org), Caffe2 is designed with expression, speed, and modularity in mind.

## Rocm_Caffe2
Rocm_Caffe2 is the official caffe2 port on AMD platform. The is made posible through HIP and ROCM software stack. AMD also provides native libraries for machine intelligent and deep learning workload. 

RCOM_Caffe2 has been validated on Ubuntu 16.04 LTS and AMD Vega 56/64/MI25; with rocm 1.71 amd MIOPEN 1.3 as of now.

### Prerequisites
* A ROCM enable platform. More info [here](https://rocm.github.io/install.html).

### ROCM_CAFFE2 Quick Start
1. [Install ROCM Software Stack](https://github.com/ROCmSoftwarePlatform/rocm_caffe2/blob/AMD_develop/rocm_docs/caffe2-install-basic.md)
2. [Install Docker](https://github.com/ROCmSoftwarePlatform/rocm_caffe2/blob/AMD_develop/rocm_docs/caffe2-docker.md)
3. [Build the Caffe2 Project](https://github.com/ROCmSoftwarePlatform/rocm_caffe2/blob/AMD_develop/rocm_docs/caffe2-build.md)
4. [Run Caffe2 Tests/Workload](https://github.com/ROCmSoftwarePlatform/rocm_caffe2/blob/AMD_develop/rocm_docs/caffe2-quickstart.md)

## Questions and Feedback

Please use Github issues (https://github.com/caffe2/caffe2/issues) to ask questions, report bugs, and request new features.

Please participate in our survey (https://www.surveymonkey.com/r/caffe2). We will send you information about new releases and special developer events/webinars.


## License

Caffe2 is released under the [Apache 2.0 license](https://github.com/caffe2/caffe2/blob/master/LICENSE). See the [NOTICE](https://github.com/caffe2/caffe2/blob/master/NOTICE) file for details.

### Further Resources on [Caffe2.ai](http://caffe2.ai)

* [Installation](http://caffe2.ai/docs/getting-started.html)
* [Learn More](http://caffe2.ai/docs/learn-more.html)
* [Upgrading to Caffe2](http://caffe2.ai/docs/caffe-migration.html)
* [Datasets](http://caffe2.ai/docs/datasets.html)
* [Model Zoo](http://caffe2.ai/docs/zoo.html)
* [Tutorials](http://caffe2.ai/docs/tutorials.html)
* [Operators Catalogue](http://caffe2.ai/docs/operators-catalogue.html)
* [C++ API](http://caffe2.ai/doxygen-c/html/classes.html)
* [Python API](http://caffe2.ai/doxygen-python/html/namespaces.html)
