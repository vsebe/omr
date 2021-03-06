/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2000, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/

#ifndef CODECACHETYPES_INCL
#define CODECACHETYPES_INCL

#include <stddef.h>                            // for size_t
#include <stdint.h>                            // for uint8_t, int32_t, etc
#include "runtime/MethodExceptionData.hpp"     // for MethodExceptionData

/*
 *  Debugging help
 */
#ifdef CODECACHE_DEBUG
#define mcc_printf if (CodeCacheDebug) printf
#define mcc_hashprintf /*printf*/
#else
#define mcc_printf
#define mcc_hashprintf
#endif

class TR_OpaqueMethodBlock;
namespace TR { class CodeCacheManager; }

namespace OMR
{

typedef void CodeCacheTrampolineCode;

struct CodeCacheErrorCode {
   // error codes are negative numbers
   enum ErrorCode
      {
      ERRORCODE_SUCCESS           = 0,
      ERRORCODE_INSUFFICIENTSPACE = -1,
      ERRORCODE_FATALERROR        = -2
      };
   };


enum CodeCacheFlags
   {
   CODECACHE_FULL_SYNC_REQUIRED =   0x00000001,   // Force synchronization of all permanent trampolines
   CODECACHE_CACHE_IS_FULL      =   0x00000002,   // Code cache is marked/considered full
   CODECACHE_TRAMP_REPORTED =       0x00000004,   // Code cache tramp region has been reported.
   CODECACHE_CCPRELOADED_REPORTED = 0x00000008,   // Code cache pre loaded code region has been reported.
   };


class CodeCacheHashEntrySlab
   {
public:
   CodeCacheHashEntrySlab() { }

   void *operator new(size_t s, CodeCacheHashEntrySlab *slab) { return slab; }

   static CodeCacheHashEntrySlab *allocate(TR::CodeCacheManager *manager, size_t slabSize);
   void free(TR::CodeCacheManager *manager);

   uint8_t                *_segment;
   uint8_t                *_heapAlloc;
   uint8_t                *_heapTop;
   CodeCacheHashEntrySlab *_next;
   };

typedef size_t CodeCacheHashKey;
struct CodeCacheHashEntry
   {
   CodeCacheHashEntry *_next;
   CodeCacheHashKey _key;
   union
      {
      struct
         {
         TR_OpaqueMethodBlock *_method;
         void *_currentStartPC;
         void *_currentTrampoline;
         } _resolved;
      struct
         {
         void *_constPool;
         int32_t _constPoolIndex;
         } _unresolved;
      } _info;
   };

class CodeCacheHashTable
   {
public:
   CodeCacheHashTable() { }

   void *operator new(size_t s, CodeCacheHashTable *table) { return table; }

   static CodeCacheHashTable *allocate(TR::CodeCacheManager *codeCacheManager);
   static size_t hashUnresolvedMethod(void *constPool, int32_t constPoolIndex);
   static size_t hashResolvedMethod(TR_OpaqueMethodBlock *method);

   CodeCacheHashEntry *findUnresolvedMethod(void *constPool, int32_t constPoolIndex);
   CodeCacheHashEntry *findResolvedMethod(TR_OpaqueMethodBlock *method);

   void dumpHashUnresolvedMethod(void);
   void reloHashUnresolvedMethod(void *constPoolOld,void *constPoolNew, int32_t cpIndex);

   void add(CodeCacheHashEntry *entry);
   bool remove(CodeCacheHashEntry *entry);

   CodeCacheHashEntry **_buckets;
   size_t               _size;
   };


struct CodeCacheTempTrampolineSyncBlock
   {
   CodeCacheHashEntry              **_hashEntryArray;  /*!< a list of hash entries representing trampolines to be synchronized */
   int32_t                           _entryCount;      /*!< Number of temp trampolines in the list */
   int32_t                           _entryListSize;   /*!< maximum size of the list */
   CodeCacheTempTrampolineSyncBlock *_next;            /*!< next list to handle overflow */
   };


struct CodeCacheMethodHeader
   {
   uint32_t _size;
   char _eyeCatcher[4];
   OMR::MethodExceptionData *_metaData;
   };

CodeCacheMethodHeader *getCodeCacheMethodHeader(char *p, int searchLimit, MethodExceptionData *metaData);


struct CodeCacheFreeCacheBlock
   {
   size_t _size;
   CodeCacheFreeCacheBlock *_next;
   };
#define MIN_SIZE_BLOCK (sizeof(CodeCacheFreeCacheBlock) > 96 ? sizeof(CodeCacheFreeCacheBlock) : 96)


struct FaintCacheBlock
   {
   FaintCacheBlock *_next;
   OMR::MethodExceptionData *_metaData;
   uint8_t _bytesToSaveAtStart;
   bool _isStillLive;
   };

#define addFreeBlock2(start, end) addFreeBlock2WithCallSite((start), (end), __FILE__, __LINE__)

}

#endif // CODECAHCETYPES_INCL
