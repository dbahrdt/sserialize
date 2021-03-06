cmake_minimum_required(VERSION 3.16)
project(sserialize C CXX)
set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

IF(CMAKE_CROSSCOMPILING)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM_BEFORE ${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM})
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH) 
	find_package(Ragel REQUIRED)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM_BEFORE})
ELSE(CMAKE_CROSSCOMPILING)
	#set(CMAKE_C_CLANG_TIDY clang-tidy;
		#-header-filter=.;
		#-checks=bugprone*,clang-analyzer*;
	#)
	#set(CMAKE_CXX_CLANG_TIDY clang-tidy;
		#-header-filter=.;
		#-checks=bugprone*,clang-analyzer*;
	#)
	find_package(LIBRT REQUIRED)
	find_package(Ragel REQUIRED)
	find_package(Cryptopp REQUIRED)
	find_package(ICU COMPONENTS uc i18n REQUIRED)
	find_package(Threads)
ENDIF(CMAKE_CROSSCOMPILING)

include(AddTargetProperties)

set(MY_COMPILE_DEFINITIONS
	"-DCGAL_HAS_THREADS"
)

option(SSERIALIZE_WAIT_ON_FAILED_ASSERTION
	"Don't abort on assertion failure but simple wait in a loop to ease debugging"
	FALSE
)
if(SSERIALIZE_WAIT_ON_FAILED_ASSERTION)
	set(MY_COMPILE_DEFINITIONS "-DSSERIALIZE_WAIT_ON_FAILED_ASSERTION" ${MY_COMPILE_DEFINITIONS})
endif()

option(
	SSERIALIZE_CONTIGUOUS_UBA_ONLY_ENABLED
	"Disable non-contiguous UByteArrayAdapter backends"
	FALSE
)
IF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_CONTIGUOUS_UBA_ONLY" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_ENABLED)

option(
	SSERIALIZE_CONTIGUOUS_UBA_ONLY_SOFT_FAIL_ENABLED
	"Fail on runtime on non-contiguous UByteArrayAdapter backends if they are disabled"
	FALSE
)
IF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_SOFT_FAIL_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_UBA_ONLY_CONTIGUOUS_SOFT_FAIL" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_CONTIGUOUS_UBA_ONLY_SOFT_FAIL_ENABLED)

option(
	SSERIALIZE_UBA_OPTIONAL_REFCOUNTING_ENABLED
	"Enable optional reference counting for UByteArrayAdapter"
	FALSE
)
	
IF (SSERIALIZE_UBA_OPTIONAL_REFCOUNTING_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_UBA_OPTIONAL_REFCOUNTING" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_UBA_OPTIONAL_REFCOUNTING_ENABLED)

option(
	SSERIALIZE_CHEAP_ASSERT_ENABLED
	"Enable cheap asserts"
	FALSE
)
IF (SSERIALIZE_CHEAP_ASSERT_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_CHEAP_ASSERT" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_CHEAP_ASSERT_ENABLED)

option(
	SSERIALIZE_NORMAL_ASSERT_ENABLED
	"Enable normal asserts"
	FALSE
)
IF (SSERIALIZE_NORMAL_ASSERT_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_NORMAL_ASSERT" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_NORMAL_ASSERT_ENABLED)

option(
	SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	"Enable expensive asserts"
	FALSE
)
IF (SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_EXPENSIVE_ASSERT" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_EXPENSIVE_ASSERT_ENABLED)

option(
	SSERIALIZE_VERY_EXPENSIVE_ASSERT_ENABLED
	"Enable expensive asserts"
	FALSE
)
IF (SSERIALIZE_VERY_EXPENSIVE_ASSERT_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DWITH_SSERIALIZE_VERY_EXPENSIVE_ASSERT" ${MY_COMPILE_DEFINITIONS})
ENDIF (SSERIALIZE_VERY_EXPENSIVE_ASSERT_ENABLED)

option(
	SSERIALIZE_INLINE_IN_LTO_ENABLED
	"Enable manual inline statements in lto builds"
	FALSE
)
IF(SSERIALIZE_INLINE_IN_LTO_ENABLED)
	set(MY_COMPILE_DEFINITIONS "-DSSERIALIZE_WITH_INLINE_IN_LTO" ${MY_COMPILE_DEFINITIONS})
ENDIF(SSERIALIZE_INLINE_IN_LTO_ENABLED)

option(SSERIALIZE_DISABLE_DTS2 "Disable dts2 support")
if ((NOT SSERIALIZE_DISABLE_DTS2) AND  (CMAKE_SIZEOF_VOID_P EQUAL 8))
	add_subdirectory(vendor/libdts2 ${PROJECT_NAME}dts2)
	
	set(MY_COMPILE_DEFINITIONS
		"-DSSERIALIZE_HAS_LIB_RATSS"
		"-DSSERIALIZE_HAS_LIB_DTS2"
		${MY_COMPILE_DEFINITIONS}
	)
	
	set(MY_DTS2_SOURCES 
		src/Static/PointOnS2.cpp
		src/spatial/PointOnS2.cpp
	)
	
	set(MY_LINK_LIBRARIES
		dts2
		${MY_LINK_LIBRARIES}
	)
endif ()

set(MY_INCLUDE_DIRS
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/vendor/libs"
	${ICU_INCLUDE_DIRS}
	${CRYPTOPP_INCLUDE_DIRS}
)

set(MY_LINK_LIBRARIES
	ICU::i18n
	ICU::uc
	${LIBRT_LIBRARIES}
	${CRYPTOPP_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${MY_LINK_LIBRARIES}
)

set(RAGEL_FLAGS "-G2")

RAGEL_PARSER(src/search/SetOpTreePrivateSimple_parser.rl)

set(STORAGE_SOURCES_CPP
	src/storage/MmappedFile.cpp
	src/storage/MmappedMemory.cpp
	src/storage/ChunkedMmappedFile.cpp
	src/storage/CompressedMmappedFile.cpp
	src/storage/UByteArrayAdapter.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivate.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterThreadSafeFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateArray.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateMmappedFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateChunkedMmappedFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateCompressedMmappedFile.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterPrivateVector.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterMM.cpp
	src/storage/UByteArrayAdapterPrivates/UByteArrayAdapterMV.cpp
	src/storage/FileHandler.cpp
	src/storage/pack_unpack_functions.cpp
	src/storage/Size.cpp
)

set(MT_SOURCES_CPP
	src/mt/MultiReaderSingleWriterLock.cpp
	src/mt/ThreadPool.cpp
)

set(STATS_SOURCES_CPP
	src/stats/histogram2d.cpp
	src/stats/ProgreessInfo.cpp
	src/stats/TimeMeasurer.cpp
)

set(STATIC_SOURCES_CPP
	src/Static/CellTextCompleter.cpp
	src/Static/TrieNodePrivates/CompactTrieNodePrivate.cpp
	src/Static/TrieNodePrivates/SimpleTrieNodePrivate.cpp
	src/Static/TrieNodePrivates/LargeCompactTrieNodePrivate.cpp
	src/Static/Set.cpp
	src/Static/StringTable.cpp
	src/Static/ItemIndexStore.cpp
	src/Static/StringCompleter.cpp
	src/Static/GeneralizedTrie.cpp
	src/Static/FlatGeneralizedTrie.cpp
	src/Static/triestats.cpp
	src/Static/HuffmanDecoder.cpp
	src/Static/KeyValueObjectStore.cpp
	src/Static/DynamicKeyValueObjectStore.cpp
	src/Static/GeoShape.cpp
	src/Static/RTree.cpp
	src/Static/HierarchyCompleter.cpp
	src/Static/GeoHierarchy.cpp
	src/Static/GeoWay.cpp
	src/Static/DenseGeoPointVector.cpp
	src/Static/GeoPolygon.cpp
	src/Static/GeoMultiPolygon.cpp
	src/Static/CQRDilator.cpp
	src/Static/UnicodeTrie/detail/SimpleNode.cpp
	src/Static/UnicodeTrie/detail/Node.cpp
	src/Static/UnicodeTrie/detail/EmptyNode.cpp
	src/Static/UnicodeTrie/FlatTrie.cpp
	src/Static/GeoHierarchySubGraph.cpp
	src/Static/Triangulation.cpp
	src/Static/TriangulationGridLocator.cpp
	src/Static/TriangulationRegionArrangement.cpp
	src/Static/TriangulationGeoHierarchyArrangement.cpp
	src/Static/TracGraph.cpp
	src/Static/Fraction.cpp
)

set(CONTAINTERS_SOURCES_CPP 
	src/containers/CompactUintArray.cpp
	src/containers/ItemIndexFactory.cpp
	src/containers/ItemIndexIterator.cpp
	src/containers/ItemIndexIteratorSetOp.cpp
	src/containers/ItemIndexIteratorIntersecting.cpp
	src/containers/MultiVarBitArray.cpp
	src/containers/DynamicBitSet.cpp
	src/containers/SortedOffsetIndex.cpp
	src/containers/SortedOffsetIndexPrivate.cpp
	src/containers/KeyValueObjectStore.cpp
	src/containers/VariantStore.cpp
	src/containers/RLEStream.cpp
	src/containers/SimpleBitVector.cpp
	src/containers/ItemIndex.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivate.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateSimple.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateStlDeque.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateStlVector.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateWAH.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateDE.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateNative.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateEliasFano.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateFoR.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateBoundedCompactUintArray.cpp
	src/containers/ItemIndexPrivates/ItemIndexPrivateRangeGenerator.cpp
)

set(SPATIAL_SOURCES_CPP
	src/spatial/GeoShape.cpp
	src/spatial/GeoNone.cpp
	src/spatial/GeoPoint.cpp
	src/spatial/GeoWay.cpp
	src/spatial/GeoRect.cpp
	src/spatial/GeoPolygon.cpp
	src/spatial/GeoMultiPolygon.cpp
	src/spatial/GeoUnionShape.cpp
	src/spatial/RTree.cpp
	src/spatial/GridRTree.cpp
	src/spatial/GeoHierarchy.cpp
	src/spatial/GridRegionTree.cpp
	src/spatial/GeoGrid.cpp
	src/spatial/DistanceCalculator.cpp
	src/spatial/LatLonCalculations.cpp
	src/spatial/CellQueryResult.cpp
	src/spatial/CellQueryResultPrivate.cpp
	src/spatial/TreedCQRImp.cpp
	src/spatial/TreedCQR.cpp
	src/spatial/CellDistance.cpp
	src/spatial/GridTextCompleter.cpp
	src/spatial/GridTreeArrangement.cpp
	src/spatial/dgg/GeoHierarchySpatialGrid.cpp
	src/spatial/dgg/HCQR.cpp
	src/spatial/dgg/HCQRIndexCompactifying.cpp
	src/spatial/dgg/HCQRIndexFromCellIndex.cpp
	src/spatial/dgg/HCQRIndexMakeStatic.cpp
	src/spatial/dgg/HCQRIndexWithCache.cpp
	src/spatial/dgg/HCQRSpatialGrid.cpp
	src/spatial/dgg/SimpleGridSpatialGrid.cpp
	src/spatial/dgg/SpatialGrid.cpp
	src/spatial/dgg/StaticHCQRSpatialGrid.cpp
	src/spatial/dgg/Static/SpatialGridInfo.cpp
	src/spatial/dgg/Static/HCQRCellInfo.cpp
	src/spatial/dgg/Static/HCQRTextIndex.cpp
	src/spatial/dgg/Static/SpatialGridRegistry.cpp
)

set(ITERATOR_SOURCES_CPP
	src/iterator/MultiBitBackInserter.cpp
	src/iterator/MultiBitIterator.cpp
	src/iterator/UDWIterator.cpp
	src/iterator/UDWIteratorPrivateHD.cpp
	src/iterator/UnaryCodeIterator.cpp
)

set(SEARCH_SOURCES_CPP
	src/search/StringCompleter.cpp
	src/search/StringCompleterPrivate.cpp
	src/search/GeoCompleter.cpp
	src/search/GeoStringCompleter.cpp
	src/search/StringCompleterPrivateMulti.cpp
	src/search/StringCompleterPrivateGeoHierarchyUnclustered.cpp
	src/search/OpTree.cpp
	src/search/SetOpTreePrivateSimple_parser.cpp
	src/search/SetOpTree.cpp
	src/search/SetOpTreePrivateComplex.cpp
	src/search/SetOpTreePrivateSimple.cpp
	src/search/SetOpTreePrivate_parser.cpp
)

set(STRINGS_SOURCES_CPP
	src/strings/stringfunctions.cpp
	src/strings/unicode_case_functions.cpp
)

set(UTILITY_SOURCES_CPP
	src/utility/log.cpp
	src/utility/Compressor.cpp
	src/utility/detail/Compressor/NoneCompressor.cpp
	src/utility/detail/Compressor/LZOCompressor.cpp
	src/utility/Fraction.cpp
	src/utility/exceptions.cpp
	src/utility/refcounting.cpp
	src/utility/debuggerfunctions.cpp
	src/utility/Bitpacking.cpp
	src/utility/assert.cpp
	src/utility/strongtypedefs.cpp
	src/algorithm/utilmath.cpp
)

set(STAGING_SOURCES_CPP
)

IF(NOT CMAKE_CROSSCOMPILING)
	set(OPTIONAL_SOURCES_CPP
		src/containers/GeneralizedTrie/SinglePassTrie.cpp
		src/strings/DiacriticRemover.cpp
	)
ENDIF(NOT CMAKE_CROSSCOMPILING)

#asan complains about unaligned load/stores
set(MY_COMPILE_DEFINITIONS "-DLZO_CFG_NO_UNALIGNED" ${MY_COMPILE_DEFINITIONS})

set(EXTERNAL_SOURCES_CPP
	vendor/libs/minilzo/minilzo.c
	vendor/libs/geographiclib/legacy/C/geodesic.c
)

set(INCLUDE_HEADERS
include/sserialize/Static/CQRDilator.h
include/sserialize/Static/Completer.h
include/sserialize/Static/DynamicFixedLengthVector.h
include/sserialize/Static/DynamicKeyValueObjectStore.h
include/sserialize/Static/DynamicVector.h
include/sserialize/Static/FlatGeneralizedTrie.h
include/sserialize/Static/Fraction.h
include/sserialize/Static/GeneralizedTrie.h
include/sserialize/Static/GeoCompleter.h
include/sserialize/Static/GeoMultiPolygon.h
include/sserialize/Static/GeoPoint.h
include/sserialize/Static/GeoPolygon.h
include/sserialize/Static/GeoWay.h
include/sserialize/Static/HieararchyCompleter.h
include/sserialize/Static/HuffmanDecoder.h
include/sserialize/Static/ItemGeoGrid.h
include/sserialize/Static/KeyValueObjectStore.h
include/sserialize/Static/Pair.h
include/sserialize/Static/PointOnS2.h
include/sserialize/Static/RGeoGrid.h
include/sserialize/Static/RTree.h
include/sserialize/Static/RTreeGeoDBCompleter.h
include/sserialize/Static/Set.h
include/sserialize/Static/StringCompleter.h
include/sserialize/Static/StringCompleterPrivates.h
include/sserialize/Static/StringTable.h
include/sserialize/Static/TracGraph.h
include/sserialize/Static/TreeNode.h
include/sserialize/Static/TriangulationGridLocator.h
include/sserialize/Static/TriangulationRegionArrangement.h
include/sserialize/Static/TrieNode.h
include/sserialize/Static/TrieNodePrivate.h
include/sserialize/Static/TrieNodePrivates/CompactTrieNodePrivate.h
include/sserialize/Static/TrieNodePrivates/LargeCompactTrieNodePrivate.h
include/sserialize/Static/TrieNodePrivates/SimpleTrieNodePrivate.h
include/sserialize/Static/TrieNodePrivates/TrieNodePrivates.h
include/sserialize/Static/UnicodeTrie/Node.h
include/sserialize/Static/UnicodeTrie/Trie.h
include/sserialize/Static/UnicodeTrie/detail/EmptyNode.h
include/sserialize/Static/UnicodeTrie/detail/SimpleNode.h
include/sserialize/Static/UnicodeTrie/FlatTrie.h
include/sserialize/Static/detail/Triangulation.h
include/sserialize/Static/GeoHierarchySubGraph.h
include/sserialize/Static/GeoShape.h
include/sserialize/Static/GeoHierarchy.h
include/sserialize/Static/Triangulation.h
include/sserialize/Static/TriangulationGeoHierarchyArrangement.h
include/sserialize/Static/TRACFGraph.h
include/sserialize/Static/CellTextCompleter.h
include/sserialize/Static/Array.h
include/sserialize/Static/DenseGeoPointVector.h
include/sserialize/Static/ItemIndexStore.h
include/sserialize/Static/Map.h
include/sserialize/Static/Version.h
include/sserialize/algorithm/find_key_in_array_functions.h
include/sserialize/algorithm/oom_algorithm.h
include/sserialize/algorithm/utilcontainerfuncs.h
include/sserialize/algorithm/utilfuncs.h
include/sserialize/algorithm/utilfunctional.h
include/sserialize/algorithm/utilsetfuncs.h
include/sserialize/algorithm/hashspecializations.h
include/sserialize/algorithm/utilmath.h
include/sserialize/containers/ArraySet.h
include/sserialize/containers/CFLArray.h
include/sserialize/containers/DirectCaches.h
include/sserialize/containers/DirectFIFOCache.h
include/sserialize/containers/DirectHugeHash.h
include/sserialize/containers/DirectHugeHashSet.h
include/sserialize/containers/DirectLFUCache.h
include/sserialize/containers/DirectLRUCache.h
include/sserialize/containers/DirectRandomCache.h
include/sserialize/containers/DynamicBitSet.h
include/sserialize/containers/GeneralizedTrie.h
include/sserialize/containers/GeneralizedTrie/BaseTrie.h
include/sserialize/containers/GeneralizedTrie/Helpers.h
include/sserialize/containers/GeneralizedTrie/SerializableTrie.h
include/sserialize/containers/GeneralizedTrie/SinglePassTrie.h
include/sserialize/containers/HuffmanTree.h
include/sserialize/containers/ItemIndexFactory.h
include/sserialize/containers/ItemIndexIterator.h
include/sserialize/containers/ItemIndexIteratorIntersecting.h
include/sserialize/containers/ItemIndexIteratorSetOp.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateBoundedCompactUintArray.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateDE.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateEliasFano.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateFoR.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivatePFoR.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRegLine.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRleDE.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateStlDeque.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateStlVector.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateWAH.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateNative.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivateRangeGenerator.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivates.h
include/sserialize/containers/ItemIndexPrivates/ItemIndexPrivate.h
include/sserialize/containers/KeyValueObjectStore.h
include/sserialize/containers/MultiVarBitArray.h
include/sserialize/containers/OOMArray.h
include/sserialize/containers/RLEStream.h
include/sserialize/containers/SortedOffsetIndex.h
include/sserialize/containers/SortedOffsetIndexPrivate.h
include/sserialize/containers/StringTable.h
include/sserialize/containers/TreeNode.h
include/sserialize/containers/UnicodeStringMap.h
include/sserialize/containers/UnicodeTrie.h
include/sserialize/containers/UnionFind.h
include/sserialize/containers/WindowedArray.h
include/sserialize/containers/AbstractArray.h
include/sserialize/containers/MMVector.h
include/sserialize/containers/LFUCache.h
include/sserialize/containers/RandomCache.h
include/sserialize/containers/CompactUintArray.h
include/sserialize/containers/HashBasedFlatTrie.h
include/sserialize/containers/ItemIndex.h
include/sserialize/containers/OADHashTable.h
include/sserialize/containers/SimpleBitVector.h
include/sserialize/containers/VariantStore.h
include/sserialize/iterator/AtStlInputIterator.h
include/sserialize/iterator/Iterator.h
include/sserialize/iterator/MultiBitBackInserter.h
include/sserialize/iterator/MultiBitIterator.h
include/sserialize/iterator/RangeGenerator.h
include/sserialize/iterator/TransformIterator.h
include/sserialize/iterator/UDWConstrainedIterator.h
include/sserialize/iterator/UDWIterator.h
include/sserialize/iterator/UDWIteratorPrivateHD.h
include/sserialize/iterator/UnaryCodeIterator.h
include/sserialize/mt/GuardedVariable.h
include/sserialize/mt/MultiReaderSingleWriterLock.h
include/sserialize/mt/ThreadPool.h
include/sserialize/search/Completer.h
include/sserialize/search/CompleterPrivate.h
include/sserialize/search/FilteredItemSet.h
include/sserialize/search/GeoCompleter.h
include/sserialize/search/GeoCompleterPrivate.h
include/sserialize/search/GeoCompleterPrivateProxy.h
include/sserialize/search/GeoStringCompleter.h
include/sserialize/search/ItemIndexIteratorDB.h
include/sserialize/search/ItemIndexIteratorGeoDB.h
include/sserialize/search/ItemSet.h
include/sserialize/search/ItemSetIterator.h
include/sserialize/search/OOMFlatTrie.h
include/sserialize/search/OpTree.h
include/sserialize/search/SetOpTree.h
include/sserialize/search/SetOpTreePrivate.h
include/sserialize/search/SetOpTreePrivateComplex.h
include/sserialize/search/SetOpTreePrivateSimple.h
include/sserialize/search/StringCompleterPrivate.h
include/sserialize/search/StringCompleterPrivateGST.h
include/sserialize/search/StringCompleterPrivateGeoHierarchyUnclustered.h
include/sserialize/search/StringCompleterPrivateMulti.h
include/sserialize/search/OOMCTCValueStore.h
include/sserialize/search/StringCompleter.h
include/sserialize/search/OOMSACTCCreator.h
include/sserialize/spatial/CellDistance.h
include/sserialize/spatial/DistanceCalculator.h
include/sserialize/spatial/GeoConstraintSetOpTreeEF.h
include/sserialize/spatial/GeoGrid.h
include/sserialize/spatial/GeoHierarchy.h
include/sserialize/spatial/GeoMultiPolygon.h
include/sserialize/spatial/GeoNone.h
include/sserialize/spatial/GeoPolygon.h
include/sserialize/spatial/GeoRectSetOpTreeFilter.h
include/sserialize/spatial/GeoRegion.h
include/sserialize/spatial/GeoRegionStore.h
include/sserialize/spatial/GeoShape.h
include/sserialize/spatial/GeoShapeFactory.h
include/sserialize/spatial/GeoUnionShape.h
include/sserialize/spatial/GeoWay.h
include/sserialize/spatial/GridRTree.h
include/sserialize/spatial/GridRegionTree.h
include/sserialize/spatial/GridTextCompleter.h
include/sserialize/spatial/GridTreeArrangement.h
include/sserialize/spatial/GrtQR.h
include/sserialize/spatial/ItemGeoGrid.h
include/sserialize/spatial/LatLonCalculations.h
include/sserialize/spatial/PointOnS2.h
include/sserialize/spatial/RTree.h
include/sserialize/spatial/RWGeoGrid.h
include/sserialize/spatial/TreedCQR.h
include/sserialize/spatial/GeoPoint.h
include/sserialize/spatial/CellQueryResult.h
include/sserialize/spatial/CellQueryResultPrivate.h
include/sserialize/spatial/GeoRect.h
include/sserialize/spatial/TreedCQRImp.h
include/sserialize/spatial/dgg/GeoHierarchySpatialGrid.h
include/sserialize/spatial/dgg/HCQR.h
include/sserialize/spatial/dgg/HCQRIndex.h
include/sserialize/spatial/dgg/HCQRIndexCompactifying.h
include/sserialize/spatial/dgg/HCQRIndexFromCellIndex.h
include/sserialize/spatial/dgg/HCQRIndexMakeStatic.h
include/sserialize/spatial/dgg/HCQRIndexWithCache.h
include/sserialize/spatial/dgg/HCQRSpatialGrid.h
include/sserialize/spatial/dgg/SimpleGridSpatialGrid.h
include/sserialize/spatial/dgg/SpatialGrid.h
include/sserialize/spatial/dgg/StaticHCQRSpatialGrid.h
include/sserialize/spatial/dgg/Static/SpatialGridInfo.h
include/sserialize/spatial/dgg/Static/HCQRCellInfo.h
include/sserialize/spatial/dgg/Static/HCQRTextIndex.h
include/sserialize/spatial/dgg/Static/SpatialGridRegistry.h
include/sserialize/stats/ProgressInfo.h
include/sserialize/stats/histogram2d.h
include/sserialize/stats/memusage.h
include/sserialize/stats/TimeMeasuerer.h
include/sserialize/stats/statfuncs.h
include/sserialize/storage/ChunkedMmappedFile.h
include/sserialize/storage/CompressedMmappedFile.h
include/sserialize/storage/Size.h
include/sserialize/storage/FlushBuffer.h
include/sserialize/storage/MmappedFile.h
include/sserialize/storage/OrderedFlushBuffer.h
include/sserialize/storage/SerializationInfo.h
include/sserialize/storage/pack_base.h
include/sserialize/storage/pack_funcs.h
include/sserialize/storage/pack_unpack_functions.h
include/sserialize/storage/FileHandler.h
include/sserialize/storage/MmappedMemory.h
include/sserialize/storage/UByteArrayAdapter.h
include/sserialize/strings/DiacriticRemover.h
include/sserialize/strings/stringfunctions.h
include/sserialize/strings/unicode_case_functions.h
include/sserialize/strings/unicode_case_table.h
include/sserialize/utility/Compressor.h
include/sserialize/utility/Fraction.h
include/sserialize/utility/debug.h
include/sserialize/utility/delegate.h
include/sserialize/utility/exceptions.h
include/sserialize/utility/log.h
include/sserialize/utility/types.h
include/sserialize/utility/refcounting.h
include/sserialize/utility/assert.h
include/sserialize/utility/checks.h
include/sserialize/utility/debuggerfunctions.h
include/sserialize/utility/Bitpacking.h
include/sserialize/utility/VersionChecker.h
include/sserialize/utility/constants.h
include/sserialize/utility/printers.h
include/sserialize/utility/type_traits.h
include/sserialize/utility/strongtypedefs.h
include/sserialize/vendor/utf8.h
include/sserialize/vendor/utf8/checked.h
include/sserialize/vendor/utf8/core.h
include/sserialize/vendor/utf8/unchecked.h
)

set(SOURCES_CPP
	${STORAGE_SOURCES_CPP}
	${MT_SOURCES_CPP}
	${STATS_SOURCES_CPP}
	${STATIC_SOURCES_CPP}
	${CONTAINTERS_SOURCES_CPP}
	${SPATIAL_SOURCES_CPP}
	${ITERATOR_SOURCES_CPP}
	${SEARCH_SOURCES_CPP}
	${STRINGS_SOURCES_CPP}
	${UTILITY_SOURCES_CPP}
	${OPTIONAL_SOURCES_CPP}
	${EXTERNAL_SOURCES_CPP}
	${STAGING_SOURCES_CPP}
	${MY_DTS2_SOURCES}
	${INCLUDE_HEADERS}
)

add_library(${PROJECT_NAME} STATIC ${SOURCES_CPP})
target_link_libraries(${PROJECT_NAME} PUBLIC ${MY_LINK_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${MY_COMPILE_DEFINITIONS})
target_compile_options(${PROJECT_NAME} PUBLIC "-Wreorder") #sserialize heavily depends on the correct initialization order
target_include_directories(${PROJECT_NAME} PUBLIC ${MY_INCLUDE_DIRS})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
add_target_properties(${PROJECT_NAME} COMPILE_FLAGS -fPIC)
set_source_files_properties(
	src/search/SetOpTreePrivateSimple_parser.cpp
	PROPERTIES
	COMPILE_OPTIONS "-Wimplicit-fallthrough=0"
)

IF(NOT CMAKE_CROSSCOMPILING)
	option(SSERIALIZE_MAKE_ALL "Build all of sserialize")
	option(SSERIALIZE_MAKE_TESTS "Build all of sserialize tests")
	option(SSERIALIZE_MAKE_TOOLS "Build all of sserialize tools")
	option(SSERIALIZE_MAKE_BENCHMARKS "Build all of sserialize BENCHMARKS")
	if (SSERIALIZE_MAKE_ALL OR SSERIALIZE_MAKE_TESTS)
		add_subdirectory(tests ${PROJECT_NAME}tests)
	endif()
	if (SSERIALIZE_MAKE_ALL OR SSERIALIZE_MAKE_TOOLS)
		add_subdirectory(tools ${PROJECT_NAME}-tools)
	endif()
	if (SSERIALIZE_MAKE_ALL OR SSERIALIZE_MAKE_BENCHMARKS)
		add_subdirectory(benchmarks ${PROJECT_NAME}benchmarks)
	endif()
ENDIF(NOT CMAKE_CROSSCOMPILING)
