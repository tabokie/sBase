# sBase

This is an implementation of relational database system. 

It mainly serves as a toy project to learn concurrency control. It also taps new structure of concurrent data structure. For now, it uses B-link structure as main storage engine. In the future, self-designed B-flow model can be implemented and tested.

Currently, it only has limited support for complete SQL syntax and incomplete consistence test.

Further plan for this project is to optimize concurrent arch and decouples some inelegant models.

### structure

* `Compiler`: Encapsulate a minimal virtual machine base on context-free grammar. The implementation explores some nice C++11 features to simplify the encoding.

* `Engine`: Middleware between front-end and storage functionality. Provide intuitive interface for most simple operation.

* `Cursor`: Performer of concrete storage operations. In B-link case, there're BFlowCursor and BPlusCursor, which respectively serve to maintain records piece and index.

* `PageManager`: Buffer manager, also controller of concurrent request. Plain C++11 api like mutex and condition_variable are used to construct more complex locks, including write-lock, read-lock, weak-write-lock.

  One more thing needed to mention is the encapsulation of page reference. In this project, I use RAII type object as entry to certain page.

* `Reflection Support`: To better styling the manipulation of runtime type, I implement weak reflection system, in which value are wrapped in uniform Value object.


