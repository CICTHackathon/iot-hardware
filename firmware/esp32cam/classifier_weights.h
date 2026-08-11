// Stage 1 in logistic regression parameters.
// Replace them by labelling events in the web interface, downloading /api/dataset.csv and running: python tools/train_classifier.py dataset.csv > classifier_weights.h
// Feature order must match kFeatureNames[] in detect.cpp.
#pragma once

#include "app_config.h"

// Provenance string shown on the web interface, so an operator can tell a
// deployed node running priors from one running weights fitted on site.
#define CLASSIFIER_ORIGIN "priors (unfitted)"

static const float kClassifierBias = -2.60f;

static const float kClassifierWeights[FEATURE_COUNT] = {
     1.40f,  // area        - changed fraction of the frame
     1.60f,  // fill        - how solidly the bounding box is filled
     1.50f,  // aspect      - taller than wide reads as a standing person
     1.00f,  // height      - bounding-box height as a fraction of the frame
    -0.50f,  // centroid_y  - movement high in the frame is more often foliage
     0.80f,  // delta       - mean luminance change over the changed cells
     1.10f,  // persistence - consecutive positive samples
    -6.00f,  // global      - near whole-frame change: lamp, AGC, door swing
};
