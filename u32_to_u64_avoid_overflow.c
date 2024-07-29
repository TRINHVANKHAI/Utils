/*
 * 32 to 64 bit convert avoid overflow
 */
uint32_t u32_count;
uint32_t u32_flag;
uint64_t u64_count;
uint64_t u64_HalfCount;

u32_count++;
if( (u32_flag ^ u32_count) & 0x80000000)
{
    u32_flag = ~u32_flag;
    u64_HalfCount += 0x80000000;
}
u64_count = u64_HalfCount | u32_count;
