#include "UmbraPlayerController.h"
#include "Audio/UmbraAudioFunctionLibrary.h"

EUmbraTransferResult AUmbraPlayerController::RequestItemTransfer(EUmbraItemTransferOperation Operation, FGuid InstanceId, EUmbraEquipmentSlot Slot, int32 TargetCell)
{
	return SubmitItemTransfer(Operation, InstanceId, Slot, TargetCell, {});
}

EUmbraTransferResult AUmbraPlayerController::SubmitItemTransfer(EUmbraItemTransferOperation Operation, FGuid InstanceId,
	EUmbraEquipmentSlot Slot, int32 TargetCell, TFunction<void(const FUmbraItemTransferResponse&)> Completion)
{
	auto* State = GetPlayerState<AUmbraPlayerState>();
	if (!State || PendingItemTransfer || NextItemTransferRequest == MAX_int32) return EUmbraTransferResult::NotReady;
	if (!HasAuthority() && !IsLocalController()) return EUmbraTransferResult::AuthorityRequired;
	FUmbraItemTransferRequest Request;
	Request.RequestId = ++NextItemTransferRequest; Request.Operation = Operation; Request.InstanceId = InstanceId;
	Request.EquipmentSlot = Slot; Request.TargetCell = TargetCell;
	Request.InventoryRevision = State->GetInventoryComponent()->GetSnapshot().Revision;
	Request.EquipmentRevision = State->GetEquipmentComponent()->GetSnapshot().Revision;
	if (HasAuthority())
	{
		const auto Response = ProcessItemTransfer(Request);
		LastItemTransferResponse = Response;
		if (Completion) Completion(Response);
		BP_ItemTransferFinished(Response);
		return Response.Result;
	}
	PendingItemTransfer = Request.RequestId;
	ItemTransferCompletion = MoveTemp(Completion);
	ServerItemTransfer(Request);
	return EUmbraTransferResult::Pending;
}

FUmbraItemTransferResponse AUmbraPlayerController::ProcessItemTransfer(const FUmbraItemTransferRequest& Request)
{
	FUmbraItemTransferResponse Response; Response.Request = Request;
	if (!HasAuthority()) { Response.Result = EUmbraTransferResult::AuthorityRequired; return Response; }
	if (Request.RequestId <= LastProcessedItemTransfer) { Response.Result = EUmbraTransferResult::StaleRequest; return Response; }
	LastProcessedItemTransfer = Request.RequestId;
	auto* State = GetPlayerState<AUmbraPlayerState>();
	if (!State || State->GetOwner() != this) return Response;
	const auto Bag = State->GetInventoryComponent()->GetSnapshot();
	const auto Equipment = State->GetEquipmentComponent()->GetSnapshot();
	if (Request.InventoryRevision != Bag.Revision || Request.EquipmentRevision != Equipment.Revision)
	{ Response.Result = EUmbraTransferResult::StaleRequest; return Response; }
	if (!Request.InstanceId.IsValid()) { Response.Result = EUmbraTransferResult::InvalidIdentity; return Response; }
	if (Request.Operation == EUmbraItemTransferOperation::Unequip)
	{
		for (const auto& Entry : Equipment.Items) if (Entry.InstanceId == Request.InstanceId) Response.Instance = Entry.GetInstance();
		FUmbraInventoryItem Returned;
		Response.Result = State->UnequipToInventorySlot(Request.EquipmentSlot, Request.InstanceId, Request.TargetCell, Returned);
	}
	else if (Request.Operation == EUmbraItemTransferOperation::Equip || Request.Operation == EUmbraItemTransferOperation::Move)
	{
		for (const auto& Entry : Bag.Items) if (Entry.InstanceId == Request.InstanceId) Response.Instance = Entry.GetInstance();
		if (Request.Operation == EUmbraItemTransferOperation::Equip) Response.Result = State->EquipFromInventory(Request.EquipmentSlot, Request.InstanceId);
		else switch (State->MoveInventoryItem(Request.InstanceId, Request.TargetCell))
		{
		case EUmbraInventoryResult::Success: Response.Result = EUmbraTransferResult::Success; break;
		case EUmbraInventoryResult::NotFound: Response.Result = EUmbraTransferResult::NotFound; break;
		case EUmbraInventoryResult::InvalidSlot: Response.Result = EUmbraTransferResult::InvalidSlot; break;
		case EUmbraInventoryResult::SlotOccupied: Response.Result = EUmbraTransferResult::SlotOccupied; break;
		default: Response.Result = EUmbraTransferResult::TransferFailed; break;
		}
	}
	else Response.Result = EUmbraTransferResult::InvalidItem;
	return Response;
}

void AUmbraPlayerController::ServerItemTransfer_Implementation(FUmbraItemTransferRequest Request)
{
	ClientItemTransferResult(ProcessItemTransfer(Request));
}

void AUmbraPlayerController::ClientItemTransferResult_Implementation(FUmbraItemTransferResponse Response)
{
	if (Response.Request.RequestId != PendingItemTransfer || !PendingItemTransfer) return;
	PendingItemTransfer = 0; LastItemTransferResponse = Response;
	auto Completion = MoveTemp(ItemTransferCompletion);
	if (Completion) Completion(Response);
	BP_ItemTransferFinished(Response);
	const auto Context = FUmbraAudioContext::FromInstance(Response.Instance);
	const auto SuccessEvent = Response.Request.Operation == EUmbraItemTransferOperation::Equip ? EUmbraAudioEvent::EquipSuccess
		: Response.Request.Operation == EUmbraItemTransferOperation::Unequip ? EUmbraAudioEvent::UnequipSuccess : EUmbraAudioEvent::InventoryTransfer;
	UUmbraAudioFunctionLibrary::PlayUI(this, Response.Result == EUmbraTransferResult::Success ? SuccessEvent : EUmbraAudioEvent::EquipFailed, Context);
}
