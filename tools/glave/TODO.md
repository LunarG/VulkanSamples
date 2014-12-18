Here is a list of all supported features in GLAVE, followed by a TODO list of features that we'd like to add soon in the development process. We've also listed the features that we'd "like to have" in the future, but don't have a short-term plan to implement. 

Feel Free to vote things up in the lists, attempt to implement them yourself, or add to the list!

As you complete an item, please copy / paste it into the SUPPORTED FEATURES section.

**SUPPORTED FEATURES IN DEBUGGER**
* Generating & loading traces
* Replay traces within the UI w/ pause, continue, stop ability
* Timeline shows CPU time of each API call
* API entrypoints names & parameters displayed in UI
* Tracing and replay standard output gets directed to Output window
* Plugin-based UI allows for extensibility to other APIs
* Search API Call Tree
  * Search result navigation
* API Call Tree Enhancements:
  * Draw call navigation buttons
  * Draw calls are shown in bold font
* Export API Calls as Text file

**TODO LIST IN DEBUGGER**
* Hide / show columns on API Call Tree
* Additional replay support:
  * Auto-pause on Validation layer error or warning
  * Single-step the replay
  * Timeline pointer gets updated in real-time of replayed API call
    * I (PeterLValve) implemented this locally, but the performance cost to update the UI was not worthwhile. This should be revisited once the replay is actually performed in a different thread.
  * Run the replay in a separate thread from the UI
  * Pop-out replay window to be floating so it can replay at larger dimensions
* Settings dialog
* State dependency graph at selected API Call
* Group API Calls by:
  * Frame boundary
  * API-specific debug groups
  * Command Buffer Submission
  * Render vs State calls
  * Trace Thread ID
* Saving 'session' data:
  * Recently loaded traces
* Capture state from replay
* Rewind the replay
* Custom viewers of each state type
* Per API entrypoint call stacks
* Collect and display machine information
* 64-bit build supports 32-bit trace files
* Timeline enhancements:
  * Pan & Zoom
  * Click call will cause API Call Tree to highlight call
* Optimize trace file loading by memory-mapping the file

**SUPPORTED FEATURES IN TRACING/REPLAYING COMMAND LINE TOOLS AND LIBRARIES**
* Command line Tracer app (glvtrace) which launches game/app with tracing library(ies) inserted and writes trace packets to a file
* Command line Tracer server which collects tracing packets over a socket connection and writes them to a file
* XGL tracer library supports multithreaded XGL apps
* Command line Replayer app (glvreplay) replays a XGL trace file with Window display on Linux

**TODO LIST IN TRACING/REPLAYING COMMAND LINE TOOLS AND LIBRARIES**
* Optimize replay speed  by using hash maps for opaque handles
* Handle XGL persistently CPU mapped buffers during tracing, rather then relying on updating data at unmap time
* Optimize Replayer speed by memory-mapping the file and/or reading file in a separate thread
* Looping in Replayer over arbitrary frames or calls
* Looping in Replayer with state restoration at beginning of loop
* Replayer window display of XGL on Windows OS
* Command line tool to display trace file in human readable format
* Command line tool for editing trace files in human readable format
* Replayer supports multithreading
* 64-bit build supports 32-bit trace files
* XGL tracing and replay cross platform support with differing GPUs

**LIKE TO HAVE FUTURE FEATURE IDEAS**
* Export trace file into *.cpp/h files that compile into a runnable application
* Editing, adding, removal of API calls
* Shader editing
* Hyperlink API Call Tree to state-specific windows
