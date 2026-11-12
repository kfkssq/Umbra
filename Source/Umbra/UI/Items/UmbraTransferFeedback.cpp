#include "UI/Items/UmbraTransferFeedback.h"

FText UmbraTransferFeedback::Message(EUmbraTransferResult Result)
{
	switch (Result)
	{
	case EUmbraTransferResult::Pending: return NSLOCTEXT("UmbraTransfer", "Pending", "Waiting for server.");
	case EUmbraTransferResult::StaleRequest: return NSLOCTEXT("UmbraTransfer", "StaleRequest", "Items changed. Refresh and try again.");
	case EUmbraTransferResult::Success: return NSLOCTEXT("UmbraTransfer", "Success", "Item transferred.");
	case EUmbraTransferResult::AuthorityRequired: return NSLOCTEXT("UmbraTransfer", "Authority", "This action requires the owning player's server request.");
	case EUmbraTransferResult::NotReady: return NSLOCTEXT("UmbraTransfer", "NotReady", "Inventory or equipment is not ready.");
	case EUmbraTransferResult::InvalidSlot: return NSLOCTEXT("UmbraTransfer", "InvalidSlot", "This item cannot use that equipment slot.");
	case EUmbraTransferResult::InvalidIdentity: return NSLOCTEXT("UmbraTransfer", "InvalidIdentity", "The item identity is invalid.");
	case EUmbraTransferResult::InvalidItem: return NSLOCTEXT("UmbraTransfer", "InvalidItem", "The item definition is invalid.");
	case EUmbraTransferResult::EquippingUnavailable: return NSLOCTEXT("UmbraTransfer", "EquippingUnavailable", "该类装备暂未开放穿戴，可保留在背包中。");
	case EUmbraTransferResult::LevelTooLow: return NSLOCTEXT("UmbraTransfer", "LevelTooLow", "Your level is too low for this item.");
	case EUmbraTransferResult::NotFound: return NSLOCTEXT("UmbraTransfer", "NotFound", "The item changed or is no longer available.");
	case EUmbraTransferResult::SlotOccupied: return NSLOCTEXT("UmbraTransfer", "SlotOccupied", "The equipment slot is occupied. Unequip its item first.");
	case EUmbraTransferResult::InventoryFull: return NSLOCTEXT("UmbraTransfer", "InventoryFull", "Your inventory is full. Equipment was kept.");
	case EUmbraTransferResult::DuplicateInstance: return NSLOCTEXT("UmbraTransfer", "DuplicateInstance", "This item identity already exists.");
	case EUmbraTransferResult::EffectRejected: return NSLOCTEXT("UmbraTransfer", "EffectRejected", "The equipment effect was rejected.");
	case EUmbraTransferResult::AppliedInvalid: return NSLOCTEXT("UmbraTransfer", "AppliedInvalid", "Item transferred, but combat stats are invalid.");
	case EUmbraTransferResult::SlotSelectionRequired: return NSLOCTEXT("UmbraTransfer", "ChooseSlot", "Choose an equipment slot for this item.");
	default: return NSLOCTEXT("UmbraTransfer", "Failed", "The item could not be transferred.");
	}
}
