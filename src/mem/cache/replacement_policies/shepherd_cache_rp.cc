
#include "mem/cache/replacement_policies/shepherd_cache_rp.hh"
#include "base/random.hh"
#include <cassert>
#include <memory>
#include "params/Shepherd.hh"
#include <iostream>

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
namespace replacement_policy
{

ShepherdReplacementPolicy :: ShepherdReplacementPolicy (const Params &p)
    : Base(p)
{
    // Initialize any parameters or data structures needed for the policy
    ShepherdInfo.FIFO_Pointer = S0;
    ShepherdInfo.CM_S0 = 0;
    ShepherdInfo.CM_S1 = 0;
    ShepherdInfo.CM_S2 = 0;
    ShepherdInfo.CM_S3 = 0;
    ShepherdInfo.InitStorageCount = 0;
    ShepherdInfoPtr = &ShepherdInfo;
}

void
ShepherdReplacementPolicy::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
{
    // Reset last touch timestamp
    std::static_pointer_cast<ShepherdReplData>(
        replacement_data)->lastTouchTick = Tick(0);

    std::static_pointer_cast<ShepherdReplData>(
        replacement_data)->tickInserted = Tick(0);
}

void
ShepherdReplacementPolicy::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::shared_ptr<ShepherdReplData> rd = std::static_pointer_cast<ShepherdReplData>(replacement_data);
    if(ShepherdInfoPtr->InitStorageCount >= 12) {
        if(rd->empty_S0) {
            rd->empty_S0 = false;
            rd->cntS0 = ShepherdInfoPtr->CM_S0;
            ShepherdInfoPtr->CM_S0++;
        }
        if(rd->empty_S1) {
            rd->empty_S1 = false;
            rd->cntS1 = ShepherdInfoPtr->CM_S1;
            ShepherdInfoPtr->CM_S1++;
        }
        if(rd->empty_S2) {
            rd->empty_S2 = false;
            rd->cntS2 = ShepherdInfoPtr->CM_S2;
            ShepherdInfoPtr->CM_S2++;
        }
        if(rd->empty_S3) {
            rd->empty_S3 = false;
            rd->cntS3 = ShepherdInfoPtr->CM_S3;
            ShepherdInfoPtr->CM_S3++;
        }
    }
    // Update last touch timestamp
    std::static_pointer_cast<ShepherdReplData>(
        replacement_data)->lastTouchTick = curTick();
}

void
ShepherdReplacementPolicy::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::shared_ptr<ShepherdReplData> rd = std::static_pointer_cast<ShepherdReplData>(replacement_data);

/*
    if(ShepherdInfoPtr->InitStorageCount < 24) {
        std::cout << "Integer: " << ShepherdInfoPtr->InitStorageCount << std::endl;
        switch(ShepherdInfoPtr->FIFO_Pointer) {
            case S0: std::cout << "S0" << std::endl; break;
            case S1: std::cout << "S1" << std::endl; break;
            case S2: std::cout << "S2" << std::endl; break;
            case S3: std::cout << "S3" << std::endl; break;
            default: std::cout << "M" << std::endl; break;
        }
    }
*/
    if(ShepherdInfoPtr->InitStorageCount < 12) {
        rd->Storage = M;
        //ShepherdInfoPtr->InitStorageCount++;
    } else {
        rd->Storage = ShepherdInfoPtr->FIFO_Pointer;
        switch(ShepherdInfoPtr->FIFO_Pointer) {
            case S0: {
                ShepherdInfoPtr->FIFO_Pointer = S1;
                ShepherdInfoPtr->CM_S0 = 0;
                break;
            }
            case S1: {
                ShepherdInfoPtr->FIFO_Pointer = S2;
                ShepherdInfoPtr->CM_S1 = 0;
                break;
            }
            case S2: {
                ShepherdInfoPtr->FIFO_Pointer = S3;
                ShepherdInfoPtr->CM_S2 = 0;
                break;
            }
            case S3: {
                ShepherdInfoPtr->FIFO_Pointer = S0;
                ShepherdInfoPtr->CM_S3 = 0;
                break;
            }
            default: {
                ShepherdInfoPtr->FIFO_Pointer = S0;
                break;
            }
        }
    }

    rd->empty_S0 = true;
    rd->empty_S1 = true;
    rd->empty_S2 = true;
    rd->empty_S3 = true;

    if(ShepherdInfoPtr->InitStorageCount < 24) {
        ShepherdInfoPtr->InitStorageCount++;
    }
    
    // Set last touch timestamp
    std::static_pointer_cast<ShepherdReplData>(
        replacement_data)->lastTouchTick = curTick();

    std::static_pointer_cast<ShepherdReplData>(
        replacement_data)->tickInserted = curTick();
}

ReplaceableEntry*
ShepherdReplacementPolicy::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

    // Visit all candidates to find victim
    ReplaceableEntry* victim = candidates[0];

    for (const auto& candidate : candidates) {
        // Update victim entry if necessary
        if (std::static_pointer_cast<ShepherdReplData>(
                    candidate->replacementData)->lastTouchTick <
                std::static_pointer_cast<ShepherdReplData>(
                    victim->replacementData)->lastTouchTick) {
            victim = candidate;
            break;
        }

        if (std::static_pointer_cast<ShepherdReplData>(
                    candidate->replacementData)->tickInserted <
                std::static_pointer_cast<ShepherdReplData>(
                    victim->replacementData)->tickInserted) {
            victim = candidate;
            break;
        }
    }

    for (const auto& candidate : candidates) {
        if((std::static_pointer_cast<ShepherdReplData>(candidate->replacementData)) -> Storage == ShepherdInfoPtr->FIFO_Pointer || 
             (std::static_pointer_cast<ShepherdReplData>(candidate->replacementData)) -> Storage == M) {
            victim = candidate;
            break;
        }
    }
    bool transfer = ((std::static_pointer_cast<ShepherdReplData>(victim->replacementData)->Storage) == M);
    bool done = false;

    for (const auto& candidate : candidates) {
        std::shared_ptr<ShepherdReplData> rd = std::static_pointer_cast<ShepherdReplData>(candidate->replacementData);
        if(rd->Storage == M || rd->Storage == ShepherdInfoPtr->FIFO_Pointer) {
            switch (ShepherdInfoPtr->FIFO_Pointer) {
                case S0 : {
                    if(rd->empty_S0) {
                        victim = candidate;
                        transfer = (rd->Storage == M);
                        done = true;
                    } else {
                        std::shared_ptr<ShepherdReplData> vic_rd = std::static_pointer_cast<ShepherdReplData>(victim->replacementData);
                        if(rd->cntS0 > vic_rd->cntS0) {
                            victim = candidate;
                            transfer = (rd->Storage == M);
                        }
                    }
                    break;
                }
                case S1 : {
                    if(rd->empty_S1) {
                        victim = candidate;
                        transfer = (rd->Storage == M);
                        done = true;
                    } else {
                        std::shared_ptr<ShepherdReplData> vic_rd = std::static_pointer_cast<ShepherdReplData>(victim->replacementData);
                        if(rd->cntS1 > vic_rd->cntS1) {
                            victim = candidate;
                            transfer = (rd->Storage == M);
                        }
                    }
                    break;
                }
                case S2 : {
                    if(rd->empty_S2) {
                        victim = candidate;
                        transfer = (rd->Storage == M);
                        done = true;
                    } else {
                        std::shared_ptr<ShepherdReplData> vic_rd = std::static_pointer_cast<ShepherdReplData>(victim->replacementData);
                        if(rd->cntS2 > vic_rd->cntS2) {
                            victim = candidate;
                            transfer = (rd->Storage == M);
                        }
                    }
                    break;
                }
                case S3 : {
                    if(rd->empty_S3) {
                        victim = candidate;
                        transfer = (rd->Storage == M);
                        done = true;
                    } else {
                        std::shared_ptr<ShepherdReplData> vic_rd = std::static_pointer_cast<ShepherdReplData>(victim->replacementData);
                        if(rd->cntS3 > vic_rd->cntS3) {
                            victim = candidate;
                            transfer = (rd->Storage == M);
                        }
                    }
                    break;
                } 
                default: break;
            }
            if(done) break;
        }
    }

    // reset all empty flags of the associated FIFO
    for(const auto& candidate : candidates) {
        std::shared_ptr<ShepherdReplData> rd = std::static_pointer_cast<ShepherdReplData>(candidate->replacementData);
        if(rd->Storage == M || rd->Storage == ShepherdInfoPtr->FIFO_Pointer) {
            switch(ShepherdInfoPtr->FIFO_Pointer) {
                case S0: rd->empty_S0 = true; break;
                case S1: rd->empty_S1 = true; break;
                case S2: rd->empty_S2 = true; break;
                case S3: rd->empty_S3 = true; break;
                default: rd->empty_S0 = true; 
                         rd->empty_S1 = true; 
                         rd->empty_S2 = true; 
                         rd->empty_S3 = true; break;
            }
        }
    }

    // transfer to main cache?
    if(transfer) {
        for(const auto& candidate : candidates) {
            std::shared_ptr<ShepherdReplData> rd = std::static_pointer_cast<ShepherdReplData>(candidate->replacementData);
            if(rd->Storage == ShepherdInfoPtr->FIFO_Pointer) {
                rd->Storage = M;
            }
        }
    }    

    return victim;
}

std::shared_ptr<ReplacementData>
ShepherdReplacementPolicy::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new ShepherdReplData());
}

} // namespace replacement_policy
} // namespace gem5
