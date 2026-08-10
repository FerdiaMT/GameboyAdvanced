# Firefox currently rejects a TextDecoder input view backed by a resizable
# WebAssembly memory buffer.  Emscripten's UTF-8 fast path is optional, so use
# its existing character-by-character fallback for those views instead.
#
# Keep this as a post-link check rather than changing the Emscripten SDK: when
# the generated runtime changes, the build fails loudly and the compatibility
# patch can be reviewed.
if(NOT DEFINED INPUT)
    message(FATAL_ERROR "INPUT must name the generated Emscripten JavaScript file.")
endif()

file(READ "${INPUT}" runtime)

set(needle
    "if(endPtr-idx>16&&heapOrArray.buffer&&UTF8Decoder){return UTF8Decoder.decode(heapOrArray.subarray(idx,endPtr))}")
set(replacement
    "if(endPtr-idx>16&&heapOrArray.buffer&&UTF8Decoder&&!heapOrArray.buffer.resizable){return UTF8Decoder.decode(heapOrArray.subarray(idx,endPtr))}")

string(FIND "${runtime}" "${needle}" match_offset)
if(match_offset EQUAL -1)
    message(FATAL_ERROR "Could not find the Emscripten UTF-8 decoder fast path in ${INPUT}.")
endif()

string(REPLACE "${needle}" "${replacement}" runtime "${runtime}")
file(WRITE "${INPUT}" "${runtime}")
