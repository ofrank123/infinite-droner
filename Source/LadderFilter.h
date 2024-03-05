#include "OliversCppHeader.h"
#include "Oscillator.h"

//- ojf: i'm building this on linux under clang, but it should compile
// fine in xcode because that also uses clang. if you're trying to compile
// this on vc++ i'm not sure....
typedef __attribute__((ext_vector_type(4))) f32 vector_f32_4;

struct LadderFilter {
    f32 res;
    f32 cutoff;
    f32 gain;
    f32 output_gain;
    f32 timestep;

    Lfo _cutoffLfo;
    Oscillator cutoffLfo;
    f32 cutoffModDepth;
    Slice<f32> cutoffMod;

    Lfo _metaCutoffLfo;
    Oscillator metaCutoffLfo;
    f32 metaCutoffModDepth;
    Slice<f32> metaCutoffMod;

    f32 prevSample = 0;
    vector_f32_4 state = {0, 0, 0, 0};
};


void processLadderFilterSamples(LadderFilter* filter, Slice<f32> input, Slice<f32> output);
