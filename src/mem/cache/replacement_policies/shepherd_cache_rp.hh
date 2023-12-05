

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_SHEPHERD_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_SHEPHERD_RP_HH__

#include "mem/cache/replacement_policies/base.hh"

namespace gem5
{

struct ShepherdParams;

GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
namespace replacement_policy
{

class ShepherdReplacementPolicy : public Base
{
  protected:
    enum LOCATION : int {
        S0 = 1,
        S1 = 2,
        S2 = 3,
        S3 = 4,
        M  = 5
    };

    /** Shepherd-specific implementation of replacement data. */
    struct ShepherdReplData : ReplacementData
    {
        /** Tick on which the entry was last touched. */
        Tick lastTouchTick;
        LOCATION Storage;
        unsigned int cntS0;
        unsigned int cntS1;
        unsigned int cntS2;
        unsigned int cntS3;
        bool empty_S0;
        bool empty_S1;
        bool empty_S2;
        bool empty_S3;
        /**
         * Default constructor. Invalidate data.
         */
        ShepherdReplData() : lastTouchTick(0), Storage(M), cntS0(1048576), cntS1(1048576), cntS2(1048576), cntS3(1048576), empty_S0(true),
                             empty_S1(true), empty_S2(true), empty_S3(true) {}
    };

    struct ShepherdInfo_t{
        LOCATION FIFO_Pointer;
        
        unsigned int CM_S0;
        unsigned int CM_S1;
        unsigned int CM_S2;
        unsigned int CM_S3;
        unsigned int InitStorageCount;
    };

    ShepherdInfo_t ShepherdInfo;
    ShepherdInfo_t* ShepherdInfoPtr;

  public:

    typedef ShepherdParams Params;
    ShepherdReplacementPolicy(const Params &p);
    ~ShepherdReplacementPolicy() = default;

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Sets its last touch tick as the starting tick.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                                    override;

    /**
     * Touch an entry to update its replacement data.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be touched.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be reset.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find replacement victim using LRU timestamps.
     *
     * @param candidates Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;

};
} // namespace replacement_policy
} // namespace gem5

#endif
