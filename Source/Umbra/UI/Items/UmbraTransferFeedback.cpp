#include "UI/Items/UmbraTransferFeedback.h"

FText UmbraTransferFeedback::Message(EUmbraTransferResult Result)
{
	switch (Result)
	{
	case EUmbraTransferResult::Success: return NSLOCTEXT("UmbraTransfer", "Success", "Item transferred.");
	case EUmbraTransferResult::AuthorityRequired: return NSLOCTEXT("UmbraTransfer", "Authority", "This action requires server authority. Remote client requests are not supported yet.");
	case EUmbraTransferResult::NotReady: return NSLOCTEXT("UmbraTransfer", "NotReady", "Inventory or equipment is not ready.");
	case EUmbraTransferResult::InvalidSlot: return NSLOCTEXT("UmbraTransfer", "InvalidSlot", "This item cannot use that equipment slot.");
	case EUmbraTransferResult::InvalidIdentity: return NSLOCTEXT("UmbraTransfer", "InvalidIdentity", "The item identity is invalid.");
	case EUmbraTransferResult::InvalidItem: return NSLOCTEXT("UmbraTransfer", "InvalidItem", "The item definition is invalid.");
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
