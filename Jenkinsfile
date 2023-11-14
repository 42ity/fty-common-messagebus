#!/usr/bin/env groovy

@Library('etn-ipm2-jenkins@featureimage/fix-cobertura-lib-CI') _

import params.CmakePipelineParams
CmakePipelineParams parameters = new CmakePipelineParams()

parameters.enableCoverity = true
etn_ipm2_build_and_tests_pipeline_cmake(parameters)

