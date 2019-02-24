
#pragma once

namespace Element {

struct WorkspaceOpenFileMessage : public AppMessage
{
    WorkspaceOpenFileMessage (const File& f) : file (f) {}
    ~WorkspaceOpenFileMessage() noexcept { }
    const File file;
};

}
