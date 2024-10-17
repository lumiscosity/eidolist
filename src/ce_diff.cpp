#include "ce_diff.h"
#include "../third_party/dtl/dtl.hpp"

#include <QList>
#include <QMessageBox>

void ce_diff(DatabaseHolder &d, DBAsset asset) {
    // Diffing happens in two steps:
    // - finding the changed data between the source copy and the patch, including identifying snippets of code around it
    // - automatically merging the code into the CE
    // This data is then used to attempt a git-styled automerge.
    // As of the time of writing this, Toucan's event editing code is extremely flaky and not ready to include here,
    // so all failed automerges need to be resolved manually with the vanilla editor; as a result, we warn on failed merges.

    // of course, why bother writing a bespoke differ when we can wheel in a generic library
    // life is good if you know where to look
    dtl::Diff3<lcf::rpg::EventCommand, std::vector<lcf::rpg::EventCommand>> diff(
        d.p_db->commonevents[asset.id].event_commands,
        d.s_db->commonevents[asset.id].event_commands,
        d.m_db->commonevents[asset.id].event_commands
    );
    diff.compose();
    if (!diff.merge()) {
        QMessageBox::warning(nullptr, "CE merge conflict", QString("CE[%1] could not be merged automatically. Please merge it manually using an external editor.").arg(asset.id));
        return;
    }
    d.m_db->commonevents[asset.id].event_commands = diff.getMergedSequence();
}
