/*
 * Copyright (c) 2021, Hunter Salyer <thefalsehonesty@gmail.com>
 * Copyright (c) 2022, Gregory Bertilson <zaggy1024@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/ByteBuffer.h>
#include <AK/Error.h>
#include <LibVideo/DecoderError.h>

#include "Parser.h"

namespace Video::VP9 {

class Decoder {
    friend class Parser;

public:
    Decoder();
    DecoderErrorOr<void> decode_frame(ByteBuffer const&);
    void dump_frame_info();

private:
    /* (8.4) Probability Adaptation Process */
    u8 merge_prob(u8 pre_prob, u8 count_0, u8 count_1, u8 count_sat, u8 max_update_factor);
    u8 merge_probs(int const* tree, int index, u8* probs, u8* counts, u8 count_sat, u8 max_update_factor);
    DecoderErrorOr<void> adapt_coef_probs();
    DecoderErrorOr<void> adapt_non_coef_probs();
    void adapt_probs(int const* tree, u8* probs, u8* counts);
    u8 adapt_prob(u8 prob, u8 counts[2]);

    /* (8.5) Prediction Processes */
    DecoderErrorOr<void> predict_intra(size_t plane, u32 x, u32 y, bool have_left, bool have_above, bool not_on_right, TXSize tx_size, u32 block_index);
    DecoderErrorOr<void> predict_inter(size_t plane, u32 x, u32 y, u32 w, u32 h, u32 block_index);

    /* (8.6) Reconstruction and Dequantization */
    DecoderErrorOr<void> reconstruct(size_t plane, u32 transform_block_x, u32 transform_block_y, TXSize transform_block_size);

    /* (8.10) Reference Frame Update Process */
    DecoderErrorOr<void> update_reference_frames();

    NonnullOwnPtr<Parser> m_parser;
};

}
