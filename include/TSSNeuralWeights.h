#ifndef TSS_NEURAL_WEIGHTS_H
#define TSS_NEURAL_WEIGHTS_H

#define TSS_NEURAL_VERSION "1.0.0"

#ifndef TSS_USE_FP16
#define TSS_USE_FP16 1
#endif

typedef struct {
#if TSS_USE_FP16
    unsigned short y[9];
    unsigned short co[9];
    unsigned short cg[9];
#else
    float y[9];
    float co[9];
    float cg[9];
#endif
    float bias;
    float scale;
} TSSNeuralLayer;

typedef struct {
    float confidence;
    float disocclusion;
    float velocity;
    float lumaDiff;
    float entropy;
} TSSNeuralInput;

typedef struct {
    float repairWeight;
    float clipMin;
    float clipMax;
    float alphaBoost;
} TSSNeuralOutput;

#define TSS_LAYER_COUNT 3
#define TSS_INPUT_FEATURES 5
#define TSS_HIDDEN_SIZE 16
#define TSS_OUTPUT_SIZE 4

typedef struct {
    TSSNeuralLayer inputLayer;
    TSSNeuralLayer hiddenLayers[TSS_LAYER_COUNT];
    TSSNeuralLayer outputLayer;
    
    float kSigmaY;
    float kSigmaChroma;
    float disocclusionThreshold;
    float velocityThreshold;
    float lumaThreshold;
    float entropyMax;
    
    float minAlpha;
    float maxAlpha;
    float repairStrength;
    float stabilizationFactor;
} TSSNeuralWeights;

#endif
