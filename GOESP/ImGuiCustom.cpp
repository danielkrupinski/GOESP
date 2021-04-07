#include "ConfigStructs.h"
#include "ImGuiCustom.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

void ImGuiCustom::colorPopup(const char* name, std::array<float, 4>& color, bool* rainbow, float* rainbowSpeed, bool* enable, float* thickness, float* rounding) noexcept
{
    ImGui::PushID(name);
    if (enable) {
        ImGui::Checkbox("##check", enable);
        ImGui::SameLine(0.0f, 5.0f);
    }
    bool openPopup = ImGui::ColorButton("##btn", color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
            color = *(std::array<float, 4>*)payload->Data;
        ImGui::EndDragDropTarget();
    }
    ImGui::SameLine(0.0f, 5.0f);
    ImGui::TextUnformatted(name);

    if (openPopup)
        ImGui::OpenPopup("##popup");

    if (ImGui::BeginPopup("##popup")) {
        ImGui::ColorPicker4("##picker", color.data(), ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar);

        if (rainbow || rainbowSpeed || thickness || rounding) {
            ImGui::SameLine();
            if (ImGui::BeginChild("##child", { 150.0f, 0.0f })) {
                if (rainbow)
                    ImGui::Checkbox("Rainbow", rainbow);
                ImGui::PushItemWidth(85.0f);
                if (rainbowSpeed)
                    ImGui::InputFloat("Speed", rainbowSpeed, 0.01f, 0.15f, "%.2f");

                if (rounding || thickness)
                    ImGui::Separator();

                if (rounding) {
                    ImGui::InputFloat("Rounding", rounding, 0.1f, 0.0f, "%.1f");
                    *rounding = std::max(*rounding, 0.0f);
                }

                if (thickness) {
                    ImGui::InputFloat("Thickness", thickness, 0.1f, 0.0f, "%.1f");
                    *thickness = std::max(*thickness, 0.0f);
                }

                ImGui::PopItemWidth();
            }
            ImGui::EndChild();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void ImGuiCustom::colorPicker(const char* name, Color& colorConfig, bool* enable, float* thickness) noexcept
{
    colorPopup(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, enable, thickness);
}

void ImGuiCustom::colorPicker(const char* name, ColorToggle& colorConfig) noexcept
{
    colorPopup(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled);
}

void ImGuiCustom::colorPicker(const char* name, ColorToggleRounding& colorConfig) noexcept
{
    colorPopup(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, nullptr, &colorConfig.rounding);
}

void ImGuiCustom::colorPicker(const char* name, ColorToggleThickness& colorConfig) noexcept
{
    colorPopup(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, &colorConfig.thickness);
}

void ImGuiCustom::colorPicker(const char* name, ColorToggleThicknessRounding& colorConfig) noexcept
{
    colorPopup(name, colorConfig.color, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.enabled, &colorConfig.thickness, &colorConfig.rounding);
}

bool ImGui::smallButtonFullWidth(const char* label, bool disabled) noexcept
{
    ImGuiContext& g = *GImGui;
    float backup_padding_y = g.Style.FramePadding.y;
    g.Style.FramePadding.y = 0.0f;
    if (disabled)
        PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    bool pressed = ButtonEx(label, ImVec2(-1, 0), ImGuiButtonFlags_AlignTextBaseLine | (disabled ? ImGuiButtonFlags_Disabled : 0));
    if (disabled)
        PopStyleVar();
    g.Style.FramePadding.y = backup_padding_y;
    return pressed;
}

void ImGui::textUnformattedCentered(const char* text) noexcept
{
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x) / 2.0f);
    ImGui::TextUnformatted(text);
}

void ImGui::progressBarFullWidth(float fraction, float height) noexcept
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2{ -1, 0 }, CalcItemWidth(), height + style.FramePadding.y * 2.0f);
    ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, 0))
        return;

    // Render
    fraction = ImSaturate(fraction);
    RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true);
    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));

    if (fraction == 0.0f)
        return;

    const ImVec2 p0{ bb.Min };
    const ImVec2 p1{ ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y };

    window->DrawList->AddQuadFilled(p0, { p1.x, p0.y }, p1, { p0.x, p1.y }, GetColorU32(ImGuiCol_PlotHistogram));
}

bool ImGui::beginTable(const char* str_id, int columns_count, ImGuiTableFlags flags, const ImVec2& outer_size, float inner_width) noexcept
{
    ImGuiID id = GetID(str_id);
    return beginTableEx(str_id, id, columns_count, flags, outer_size, inner_width);
}

static const float TABLE_BORDER_SIZE = 1.0f;
inline ImGuiTableFlags TableFixFlags(ImGuiTableFlags flags, ImGuiWindow* outer_window)
{
    // Adjust flags: set default sizing policy
    if ((flags & ImGuiTableFlags_SizingMask_) == 0)
        flags |= ((flags & ImGuiTableFlags_ScrollX) || (outer_window->Flags & ImGuiWindowFlags_AlwaysAutoResize)) ? ImGuiTableFlags_SizingFixedFit : ImGuiTableFlags_SizingStretchSame;

    // Adjust flags: enable NoKeepColumnsVisible when using ImGuiTableFlags_SizingFixedSame
    if ((flags & ImGuiTableFlags_SizingMask_) == ImGuiTableFlags_SizingFixedSame)
        flags |= ImGuiTableFlags_NoKeepColumnsVisible;

    // Adjust flags: enforce borders when resizable
    if (flags & ImGuiTableFlags_Resizable)
        flags |= ImGuiTableFlags_BordersInnerV;

    // Adjust flags: disable NoHostExtendX/NoHostExtendY if we have any scrolling going on
    if (flags & (ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
        flags &= ~(ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoHostExtendY);

    // Adjust flags: NoBordersInBodyUntilResize takes priority over NoBordersInBody
    if (flags & ImGuiTableFlags_NoBordersInBodyUntilResize)
        flags &= ~ImGuiTableFlags_NoBordersInBody;

    // Adjust flags: disable saved settings if there's nothing to save
    if ((flags & (ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable)) == 0)
        flags |= ImGuiTableFlags_NoSavedSettings;

    // Inherit _NoSavedSettings from top-level window (child windows always have _NoSavedSettings set)
#ifdef IMGUI_HAS_DOCK
    ImGuiWindow* window_for_settings = outer_window->RootWindowDockStop;
#else
    ImGuiWindow* window_for_settings = outer_window->RootWindow;
#endif
    if (window_for_settings->Flags & ImGuiWindowFlags_NoSavedSettings)
        flags |= ImGuiTableFlags_NoSavedSettings;

    return flags;
}

// added: child_flags |= (g.CurrentWindow->Flags & ImGuiWindowFlags_NoInputs);
bool ImGui::beginTableEx(const char* name, ImGuiID id, int columns_count, ImGuiTableFlags flags, const ImVec2& outer_size, float inner_width) noexcept
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* outer_window = GetCurrentWindow();
    if (outer_window->SkipItems) // Consistent with other tables + beneficial side effect that assert on miscalling EndTable() will be more visible.
        return false;

    // Sanity checks
    IM_ASSERT(columns_count > 0 && columns_count <= IMGUI_TABLE_MAX_COLUMNS && "Only 1..64 columns allowed!");
    if (flags & ImGuiTableFlags_ScrollX)
        IM_ASSERT(inner_width >= 0.0f);

    // If an outer size is specified ahead we will be able to early out when not visible. Exact clipping rules may evolve.
    const bool use_child_window = (flags & (ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) != 0;
    const ImVec2 avail_size = GetContentRegionAvail();
    ImVec2 actual_outer_size = CalcItemSize(outer_size, ImMax(avail_size.x, 1.0f), use_child_window ? ImMax(avail_size.y, 1.0f) : 0.0f);
    ImRect outer_rect(outer_window->DC.CursorPos, outer_window->DC.CursorPos + actual_outer_size);
    if (use_child_window && IsClippedEx(outer_rect, 0, false))
    {
        ItemSize(outer_rect);
        return false;
    }

    // Acquire storage for the table
    ImGuiTable* table = g.Tables.GetOrAddByKey(id);
    const int instance_no = (table->LastFrameActive != g.FrameCount) ? 0 : table->InstanceCurrent + 1;
    const ImGuiID instance_id = id + instance_no;
    const ImGuiTableFlags table_last_flags = table->Flags;
    if (instance_no > 0)
        IM_ASSERT(table->ColumnsCount == columns_count && "BeginTable(): Cannot change columns count mid-frame while preserving same ID");

    // Fix flags
    table->IsDefaultSizingPolicy = (flags & ImGuiTableFlags_SizingMask_) == 0;
    flags = TableFixFlags(flags, outer_window);

    // Initialize
    table->ID = id;
    table->Flags = flags;
    table->InstanceCurrent = (ImS16)instance_no;
    table->LastFrameActive = g.FrameCount;
    table->OuterWindow = table->InnerWindow = outer_window;
    table->ColumnsCount = columns_count;
    table->IsLayoutLocked = false;
    table->InnerWidth = inner_width;
    table->UserOuterSize = outer_size;

    // When not using a child window, WorkRect.Max will grow as we append contents.
    if (use_child_window)
    {
        // Ensure no vertical scrollbar appears if we only want horizontal one, to make flag consistent
        // (we have no other way to disable vertical scrollbar of a window while keeping the horizontal one showing)
        ImVec2 override_content_size(FLT_MAX, FLT_MAX);
        if ((flags & ImGuiTableFlags_ScrollX) && !(flags & ImGuiTableFlags_ScrollY))
            override_content_size.y = FLT_MIN;

        // Ensure specified width (when not specified, Stretched columns will act as if the width == OuterWidth and
        // never lead to any scrolling). We don't handle inner_width < 0.0f, we could potentially use it to right-align
        // based on the right side of the child window work rect, which would require knowing ahead if we are going to
        // have decoration taking horizontal spaces (typically a vertical scrollbar).
        if ((flags & ImGuiTableFlags_ScrollX) && inner_width > 0.0f)
            override_content_size.x = inner_width;

        if (override_content_size.x != FLT_MAX || override_content_size.y != FLT_MAX)
            SetNextWindowContentSize(ImVec2(override_content_size.x != FLT_MAX ? override_content_size.x : 0.0f, override_content_size.y != FLT_MAX ? override_content_size.y : 0.0f));

        // Reset scroll if we are reactivating it
        if ((table_last_flags & (ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) == 0)
            SetNextWindowScroll(ImVec2(0.0f, 0.0f));

        // Create scrolling region (without border and zero window padding)
        ImGuiWindowFlags child_flags = (flags & ImGuiTableFlags_ScrollX) ? ImGuiWindowFlags_HorizontalScrollbar : ImGuiWindowFlags_None;
        child_flags |= (g.CurrentWindow->Flags & ImGuiWindowFlags_NoInputs);
        BeginChildEx(name, instance_id, outer_rect.GetSize(), false, child_flags);
        table->InnerWindow = g.CurrentWindow;
        table->WorkRect = table->InnerWindow->WorkRect;
        table->OuterRect = table->InnerWindow->Rect();
        table->InnerRect = table->InnerWindow->InnerRect;
        IM_ASSERT(table->InnerWindow->WindowPadding.x == 0.0f && table->InnerWindow->WindowPadding.y == 0.0f && table->InnerWindow->WindowBorderSize == 0.0f);
    }
    else
    {
        // For non-scrolling tables, WorkRect == OuterRect == InnerRect.
        // But at this point we do NOT have a correct value for .Max.y (unless a height has been explicitly passed in). It will only be updated in EndTable().
        table->WorkRect = table->OuterRect = table->InnerRect = outer_rect;
    }

    // Push a standardized ID for both child-using and not-child-using tables
    PushOverrideID(instance_id);

    // Backup a copy of host window members we will modify
    ImGuiWindow* inner_window = table->InnerWindow;
    table->HostIndentX = inner_window->DC.Indent.x;
    table->HostClipRect = inner_window->ClipRect;
    table->HostSkipItems = inner_window->SkipItems;
    table->HostBackupWorkRect = inner_window->WorkRect;
    table->HostBackupParentWorkRect = inner_window->ParentWorkRect;
    table->HostBackupColumnsOffset = outer_window->DC.ColumnsOffset;
    table->HostBackupPrevLineSize = inner_window->DC.PrevLineSize;
    table->HostBackupCurrLineSize = inner_window->DC.CurrLineSize;
    table->HostBackupCursorMaxPos = inner_window->DC.CursorMaxPos;
    table->HostBackupItemWidth = outer_window->DC.ItemWidth;
    table->HostBackupItemWidthStackSize = outer_window->DC.ItemWidthStack.Size;
    inner_window->DC.PrevLineSize = inner_window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);

    // Padding and Spacing
    // - None               ........Content..... Pad .....Content........
    // - PadOuter           | Pad ..Content..... Pad .....Content.. Pad |
    // - PadInner           ........Content.. Pad | Pad ..Content........
    // - PadOuter+PadInner  | Pad ..Content.. Pad | Pad ..Content.. Pad |
    const bool pad_outer_x = (flags & ImGuiTableFlags_NoPadOuterX) ? false : (flags & ImGuiTableFlags_PadOuterX) ? true : (flags & ImGuiTableFlags_BordersOuterV) != 0;
    const bool pad_inner_x = (flags & ImGuiTableFlags_NoPadInnerX) ? false : true;
    const float inner_spacing_for_border = (flags & ImGuiTableFlags_BordersInnerV) ? TABLE_BORDER_SIZE : 0.0f;
    const float inner_spacing_explicit = (pad_inner_x && (flags & ImGuiTableFlags_BordersInnerV) == 0) ? g.Style.CellPadding.x : 0.0f;
    const float inner_padding_explicit = (pad_inner_x && (flags & ImGuiTableFlags_BordersInnerV) != 0) ? g.Style.CellPadding.x : 0.0f;
    table->CellSpacingX1 = inner_spacing_explicit + inner_spacing_for_border;
    table->CellSpacingX2 = inner_spacing_explicit;
    table->CellPaddingX = inner_padding_explicit;
    table->CellPaddingY = g.Style.CellPadding.y;

    const float outer_padding_for_border = (flags & ImGuiTableFlags_BordersOuterV) ? TABLE_BORDER_SIZE : 0.0f;
    const float outer_padding_explicit = pad_outer_x ? g.Style.CellPadding.x : 0.0f;
    table->OuterPaddingX = (outer_padding_for_border + outer_padding_explicit) - table->CellPaddingX;

    table->CurrentColumn = -1;
    table->CurrentRow = -1;
    table->RowBgColorCounter = 0;
    table->LastRowFlags = ImGuiTableRowFlags_None;
    table->InnerClipRect = (inner_window == outer_window) ? table->WorkRect : inner_window->ClipRect;
    table->InnerClipRect.ClipWith(table->WorkRect);     // We need this to honor inner_width
    table->InnerClipRect.ClipWithFull(table->HostClipRect);
    table->InnerClipRect.Max.y = (flags & ImGuiTableFlags_NoHostExtendY) ? ImMin(table->InnerClipRect.Max.y, inner_window->WorkRect.Max.y) : inner_window->ClipRect.Max.y;

    table->RowPosY1 = table->RowPosY2 = table->WorkRect.Min.y; // This is needed somehow
    table->RowTextBaseline = 0.0f; // This will be cleared again by TableBeginRow()
    table->FreezeRowsRequest = table->FreezeRowsCount = 0; // This will be setup by TableSetupScrollFreeze(), if any
    table->FreezeColumnsRequest = table->FreezeColumnsCount = 0;
    table->IsUnfrozenRows = true;
    table->DeclColumnsCount = 0;

    // Using opaque colors facilitate overlapping elements of the grid
    table->BorderColorStrong = GetColorU32(ImGuiCol_TableBorderStrong);
    table->BorderColorLight = GetColorU32(ImGuiCol_TableBorderLight);

    // Make table current
    const int table_idx = g.Tables.GetIndex(table);
    g.CurrentTableStack.push_back(ImGuiPtrOrIndex(table_idx));
    g.CurrentTable = table;
    outer_window->DC.CurrentTableIdx = table_idx;
    if (inner_window != outer_window) // So EndChild() within the inner window can restore the table properly.
        inner_window->DC.CurrentTableIdx = table_idx;

    if ((table_last_flags & ImGuiTableFlags_Reorderable) && (flags & ImGuiTableFlags_Reorderable) == 0)
        table->IsResetDisplayOrderRequest = true;

    // Mark as used
    if (table_idx >= g.TablesLastTimeActive.Size)
        g.TablesLastTimeActive.resize(table_idx + 1, -1.0f);
    g.TablesLastTimeActive[table_idx] = (float)g.Time;
    table->MemoryCompacted = false;

    // Setup memory buffer (clear data if columns count changed)
    const int stored_size = table->Columns.size();
    if (stored_size != 0 && stored_size != columns_count)
    {
        IM_FREE(table->RawData);
        table->RawData = NULL;
    }
    if (table->RawData == NULL)
    {
        TableBeginInitMemory(table, columns_count);
        table->IsInitializing = table->IsSettingsRequestLoad = true;
    }
    if (table->IsResetAllRequest)
        TableResetSettings(table);
    if (table->IsInitializing)
    {
        // Initialize
        table->SettingsOffset = -1;
        table->IsSortSpecsDirty = true;
        table->InstanceInteracted = -1;
        table->ContextPopupColumn = -1;
        table->ReorderColumn = table->ResizedColumn = table->LastResizedColumn = -1;
        table->AutoFitSingleColumn = -1;
        table->HoveredColumnBody = table->HoveredColumnBorder = -1;
        for (int n = 0; n < columns_count; n++)
        {
            ImGuiTableColumn* column = &table->Columns[n];
            float width_auto = column->WidthAuto;
            *column = ImGuiTableColumn();
            column->WidthAuto = width_auto;
            column->IsPreserveWidthAuto = true; // Preserve WidthAuto when reinitializing a live table: not technically necessary but remove a visible flicker
            column->DisplayOrder = table->DisplayOrderToIndex[n] = (ImGuiTableColumnIdx)n;
            column->IsEnabled = column->IsEnabledNextFrame = true;
        }
    }

    // Load settings
    if (table->IsSettingsRequestLoad)
        TableLoadSettings(table);

    // Handle DPI/font resize
    // This is designed to facilitate DPI changes with the assumption that e.g. style.CellPadding has been scaled as well.
    // It will also react to changing fonts with mixed results. It doesn't need to be perfect but merely provide a decent transition.
    // FIXME-DPI: Provide consistent standards for reference size. Perhaps using g.CurrentDpiScale would be more self explanatory.
    // This is will lead us to non-rounded WidthRequest in columns, which should work but is a poorly tested path.
    const float new_ref_scale_unit = g.FontSize; // g.Font->GetCharAdvance('A') ?
    if (table->RefScale != 0.0f && table->RefScale != new_ref_scale_unit)
    {
        const float scale_factor = new_ref_scale_unit / table->RefScale;
        //IMGUI_DEBUG_LOG("[table] %08X RefScaleUnit %.3f -> %.3f, scaling width by %.3f\n", table->ID, table->RefScaleUnit, new_ref_scale_unit, scale_factor);
        for (int n = 0; n < columns_count; n++)
            table->Columns[n].WidthRequest = table->Columns[n].WidthRequest * scale_factor;
    }
    table->RefScale = new_ref_scale_unit;

    // Disable output until user calls TableNextRow() or TableNextColumn() leading to the TableUpdateLayout() call..
    // This is not strictly necessary but will reduce cases were "out of table" output will be misleading to the user.
    // Because we cannot safely assert in EndTable() when no rows have been created, this seems like our best option.
    inner_window->SkipItems = true;

    // Clear names
    // At this point the ->NameOffset field of each column will be invalid until TableUpdateLayout() or the first call to TableSetupColumn()
    if (table->ColumnsNames.Buf.Size > 0)
        table->ColumnsNames.Buf.resize(0);

    // Apply queued resizing/reordering/hiding requests
    TableBeginApplyRequests(table);

    return true;
}

void ImGui::textEllipsisInTableCell(const char* text) noexcept
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImGuiTable* table = g.CurrentTable;
    IM_ASSERT(table != NULL && "Need to call textEllipsisInTableCell() after BeginTable()!");
    IM_ASSERT(table->CurrentColumn != -1);

    const char* textEnd = FindRenderedTextEnd(text);
    ImVec2 textSize = CalcTextSize(text, textEnd, true);
    ImVec2 textPos = window->DC.CursorPos;
    float textHeight = ImMax(textSize.y, table->RowMinHeight - table->CellPaddingY * 2.0f);

    float ellipsisMax = TableGetCellBgRect(table, table->CurrentColumn).Max.x;
    RenderTextEllipsis(window->DrawList, textPos, ImVec2(ellipsisMax, textPos.y + textHeight + g.Style.FramePadding.y), ellipsisMax, ellipsisMax, text, textEnd, &textSize);

    ImRect bb(textPos, textPos + textSize);
    ItemSize(textSize, 0.0f);
    ItemAdd(bb, 0);
}
