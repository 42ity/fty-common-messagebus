#!/usr/bin/env groovy

@Library('etn-ipm2-jenkins') _

import params.CmakePipelineParams
CmakePipelineParams parameters = new CmakePipelineParams()
//parameters.debugBuildRunCoverage = true
parameters.debugBuildRunTests = false
parameters.debugBuildRunMemcheck = false

etn_ipm2_build_and_tests_pipeline_cmake(parameters)

