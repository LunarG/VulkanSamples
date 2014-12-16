Here is a list of all supported features in GLAVE, followed by a TODO list of features that we'd like to add soon in the development process. We've also listed the features that we'd "like to have" in the future, but don't have a short-term plan to implement. 

Feel Free to vote things up in the lists, attempt to implement them yourself, or add to the list!

As you complete an item, please copy / paste it into the SUPPORTED FEATURES section.

**SUPPORTED FEATURES**
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
* Export API Calls as Text file

**TODO LIST**
* API Call Tree Enhancements:
  * Bold draw calls
* Hide / show columns on API Call Tree
* Additional replay support:
  * Auto-pause on Validation layer error or warning
  * Single-step the replay
  * Timeline pointer gets updated in real-time of replayed API call
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

**LIKE TO HAVE FUTURE FEATURE IDEAS**
* Export trace file into *.cpp/h files that compile into a runnable application
* Editing, adding, removal of API calls
* Shader editing
* Hyperlink API Call Tree to state-specific windows
