/*
 * Copyright (C) 2004-2018 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "economy/flag.h"

#include "base/macros.h"
#include "base/wexception.h"
#include "economy/economy.h"
#include "economy/portdock.h"
#include "economy/request.h"
#include "economy/road.h"
#include "economy/ware_instance.h"
#include "logic/editor_game_base.h"
#include "logic/game.h"
#include "logic/map_objects/map_object.h"
#include "logic/map_objects/tribes/building.h"
#include "logic/map_objects/tribes/tribe_descr.h"
#include "logic/map_objects/tribes/warehouse.h"
#include "logic/map_objects/tribes/worker.h"
#include "logic/player.h"

namespace {
const Widelands::Coords test_coords(199, 199);
const uint32_t test_coords_hash = test_coords.hash();

} // namespace

namespace Widelands {

FlagDescr g_flag_descr("flag", "Flag");

const FlagDescr& Flag::descr() const {
	return g_flag_descr;
}
// NOCOM https://bazaar.launchpad.net/~widelands-dev/widelands/trunk/revision/8775

/**
 * A bare flag, used for testing only.
 */
Flag::Flag()
   : PlayerImmovable(g_flag_descr),
     animstart_(0),
     building_(nullptr),
     ware_capacity_(8),
     ware_filled_(0),
     wares_(new PendingWare[ware_capacity_]),
     always_call_for_flag_(nullptr),
	 last_update_(0),
	 freeze_counter_(0) {
	for (uint32_t i = 0; i < 6; ++i) {
		roads_[i] = nullptr;
	}
}

/**
 * Shouldn't be necessary to do anything,
 * since die() always calls cleanup() first.
 */
Flag::~Flag() {
	if (ware_filled_) {
		log("Flag: ouch! wares left\n");
	}
	delete[] wares_;

	if (building_) {
		log("Flag: ouch! building left\n");
	}

	if (flag_jobs_.size()) {
		log("Flag: ouch! flagjobs left\n");
	}

	for (int32_t i = 0; i < 6; ++i) {
		if (roads_[i]) {
			log("Flag: ouch! road left\n");
		}
	}
}

void Flag::load_finish(EditorGameBase& egbase) {
	auto should_be_deleted = [&egbase, this](const OPtr<Worker>& r) {
		Worker& worker = *r.get(egbase);
		Bob::State const* const state = worker.get_state(Worker::taskWaitforcapacity);
		if (state == nullptr) {
			log("WARNING: worker %u is in the capacity wait queue of flag %u but "
			    "does not have a waitforcapacity task! Removing from queue.\n",
			    worker.serial(), serial());
			return true;
		}
		if (state->objvar1 != this) {
			log("WARNING: worker %u is in the capacity wait queue of flag %u but "
			    "its waitforcapacity task is for map object %u! Removing from "
			    "queue.\n",
			    worker.serial(), serial(), state->objvar1.serial());
			return true;
		}
		return false;
	};

	capacity_wait_.erase(
	   std::remove_if(capacity_wait_.begin(), capacity_wait_.end(), should_be_deleted),
	   capacity_wait_.end());
}

/**
 * Creates a flag at the given location.
 */
Flag::Flag(EditorGameBase& egbase, Player* owning_player, const Coords& coords, Economy* eco)
   : PlayerImmovable(g_flag_descr),
     building_(nullptr),
     ware_capacity_(8),
     ware_filled_(0),
     wares_(new PendingWare[ware_capacity_]),
     always_call_for_flag_(nullptr),
	 last_update_(egbase.get_gametime()) {
	for (uint32_t i = 0; i < 6; ++i) {
		roads_[i] = nullptr;
	}

	set_owner(owning_player);

	set_flag_position(coords);

	upcast(Road, road, egbase.map().get_immovable(coords));
	upcast(Game, game, &egbase);

	if (game) {
		if (eco) {
			// We're saveloading
			eco->add_flag(*this);
		} else {
			//  we split a road, or a new, standalone flag is created
			(road ? road->get_economy() : owning_player->create_economy())->add_flag(*this);
			if (road) {
				road->presplit(*game, coords);
			}
		}
	}

	init(egbase);

	if (!eco && road && game) {
		road->postsplit(*game, *this);
	}
}

void Flag::set_flag_position(Coords coords) {
	position_ = coords;
}

int32_t Flag::get_size() const {
	return SMALL;
}

bool Flag::get_passable() const {
	return true;
}

Flag& Flag::base_flag() {
	return *this;
}

/**
 * Call this only from Economy code!
 */
void Flag::set_economy(Economy* const e) {
	Economy* const old = get_economy();

	if (old == e) {
		return;
	}

	PlayerImmovable::set_economy(e);

	for (int32_t i = 0; i < ware_filled_; ++i) {
		wares_[i].ware->set_economy(e);
	}

	if (building_) {
		building_->set_economy(e);
	}

	for (const FlagJob& temp_job : flag_jobs_) {
		temp_job.request->set_economy(e);
	}

	for (int8_t i = 0; i < 6; ++i) {
		if (roads_[i]) {
			roads_[i]->set_economy(e);
		}
	}
}

/**
 * Call this only from the Building init!
 */
void Flag::attach_building(EditorGameBase& egbase, Building& building) {
	last_update_ = egbase.get_gametime();
	assert(!building_ || building_ == &building);

	building_ = &building;

	const Map& map = egbase.map();
	egbase.set_road(
	   map.get_fcoords(map.tl_n(position_)), RoadType::kSouthEast,
	   building_->get_size() == BaseImmovable::SMALL ? RoadType::kNormal : RoadType::kBusy);

	building.set_economy(get_economy());
}

/**
 * Call this only from the Building cleanup!
 */
void Flag::detach_building(EditorGameBase& egbase) {
	last_update_ = egbase.get_gametime();
	assert(building_);

	building_->set_economy(nullptr);

	const Map& map = egbase.map();
	egbase.set_road(map.get_fcoords(map.tl_n(position_)), RoadType::kSouthEast, RoadType::kNone);

	building_ = nullptr;
}

/**
 * Call this only from the Road init!
 */
void Flag::attach_road(int32_t const dir, Road* const road) {
	assert(!roads_[dir - 1] || roads_[dir - 1] == road);

	roads_[dir - 1] = road;
	roads_[dir - 1]->set_economy(get_economy());
}

/**
 * Call this only from the Road init!
 */
void Flag::detach_road(int32_t const dir) {
	assert(roads_[dir - 1]);

	roads_[dir - 1]->set_economy(nullptr);
	roads_[dir - 1] = nullptr;
}

/**
 * \return all positions we occupy on the map. For a Flag, this is only one.
 */
BaseImmovable::PositionList Flag::get_positions(const EditorGameBase&) const {
	PositionList rv;
	rv.push_back(position_);
	return rv;
}

/**
 * \return neighbouring flags.
 */
void Flag::get_neighbours(WareWorker type, RoutingNodeNeighbours& neighbours) {
	for (int8_t i = 0; i < 6; ++i) {
		Road* const road = roads_[i];
		if (!road) {
			continue;
		}

		Flag* f = &road->get_flag(Road::FlagEnd);
		int32_t nb_cost;
		if (f != this) {
			nb_cost = road->get_cost(Road::FlagStart);
		} else {
			f = &road->get_flag(Road::FlagStart);
			nb_cost = road->get_cost(Road::FlagEnd);
		}
		if (type == wwWARE) {
			nb_cost += nb_cost * (get_waitcost() + f->get_waitcost()) / 2;
		}
		RoutingNodeNeighbour n(f, nb_cost);

		assert(n.get_neighbour() != this);
		neighbours.push_back(n);
	}

	if (building_ && building_->descr().get_isport()) {
		Warehouse* wh = static_cast<Warehouse*>(building_);
		if (PortDock* pd = wh->get_portdock()) {
			pd->add_neighbours(neighbours);
		}
	}
}

/**
 * \return the road that leads to the given flag.
 */
Road* Flag::get_road(Flag& flag) {
	for (int8_t i = 0; i < 6; ++i) {
		if (Road* const road = roads_[i]) {
			if (&road->get_flag(Road::FlagStart) == &flag || &road->get_flag(Road::FlagEnd) == &flag) {
				return road;
			}
		}
	}
	return nullptr;
}

/// \return the number of roads connected to the flag.
uint8_t Flag::nr_of_roads() const {
	uint8_t counter = 0;
	for (uint8_t road_id = 6; road_id; --road_id) {
		if (get_road(road_id) != nullptr) {
			++counter;
		}
	}
	return counter;
}

bool Flag::is_dead_end() const {
	if (get_building()) {
		return false;
	}
	Flag const* first_other_flag = nullptr;
	for (uint8_t road_id = 6; road_id; --road_id) {
		if (Road* const road = get_road(road_id)) {
			Flag& start = road->get_flag(Road::FlagStart);
			Flag& other = this == &start ? road->get_flag(Road::FlagEnd) : start;
			if (first_other_flag) {
				if (&other != first_other_flag)
					return false;
			} else {
				first_other_flag = &other;
			}
		}
	}
	return true;
}

/**
 * Called by workers wanting to drop a ware to their building's flag.
 * \return true/allow on low congestion-risk.
 */
bool Flag::has_capacity_for_ware(WareInstance& ware) const {
	bool test = false;
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM has_capacity_for_ware (%d, %d) %s - this\n", get_position().x, get_position().y, ware.descr().name().c_str());
		test = true;
	}
	if (abs(get_position().x - test_coords.x) <= 1 && abs(get_position().y - test_coords.y) <= 1) {
		log("NOCOM has_capacity_for_ware (%d, %d) %s - very near flag\n", get_position().x, get_position().y, ware.descr().name().c_str());
		test = true;
	}
	/*else if (get_position().x == test_coords.x - 2 || get_position().x == test_coords.x + 2 || get_position().y == test_coords.y - 2 || get_position().y == test_coords.y + 2) {
		log("NOCOM has_capacity_for_ware (%d, %d) %s - near flag\n", get_position().x, get_position().y, ware.descr().name().c_str());
		test = true;
	}
	*/
	// NOCOM check this
	// avoid iteration for the easy cases
	if (ware_filled_ < ware_capacity_ - 2) {
		if (test) log("NOCOM ware_filled_ < ware_capacity_ - 2 true\n");
		return true;  // more than two free slots, allow
	}
	if (ware_filled_ >= ware_capacity_) {
		if (test) log("NOCOM ware_filled_ >= ware_capacity_ false\n");
		return false;  // all slots filled, no room
	}

	DescriptionIndex const descr_index = ware.descr_index();
	for (int i = 0; i < ware_filled_; ++i) {
		if (wares_[i].ware->descr_index() == descr_index) {
			if (test) log("NOCOM already there - false\n");
			return false;  // ware of this type already present, leave room for other types
		}
	}
	if (test) log("NOCOM end - true\n");
	return true;  // ware of this type not present, allow
}

/**
 * \return true/allow if the flag can hold more wares of any type.
 * has_capacity_for_ware() also checks ware's type to prevent congestion.
 */
bool Flag::has_capacity() const {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::has_capacity (%d, %d)\n", get_position().x, get_position().y);
	}
	return (ware_filled_ < ware_capacity_);
}

/**
 * Signal the given bob by interrupting its task as soon as capacity becomes
 * free.
 *
 * The capacity queue is a simple FIFO queue.
 */
void Flag::wait_for_capacity(Game& game, Worker& bob) {
	last_update_ = game.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::wait_for_capacity (%d, %d)\n", get_position().x, get_position().y);
	}
	capacity_wait_.push_back(&bob);
}

/**
 * Remove the worker from the list of workers waiting for free capacity.
 */
void Flag::skip_wait_for_capacity(Game& game, Worker& w) {
	last_update_ = game.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::skip_wait_for_capacity (%d, %d)\n", get_position().x, get_position().y);
	}
	CapacityWaitQueue::iterator const it =
	   std::find(capacity_wait_.begin(), capacity_wait_.end(), &w);
	if (it != capacity_wait_.end())
		capacity_wait_.erase(it);
}

/**
 * Adds given ware to this flag.
 * Please check has_capacity() or better has_capacity_for_ware() before.
 */
void Flag::add_ware(EditorGameBase& egbase, WareInstance& ware) {
	last_update_ = egbase.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::add_ware (%d, %d)\n", get_position().x, get_position().y);
	}
	assert(ware_filled_ < ware_capacity_);
	init_ware(egbase, ware, wares_[ware_filled_++]);
	if (upcast(Game, game, &egbase)) {
		ware.update(*game);  //  will call call_carrier() if necessary
	}
}

/**
 * Properly assigns given ware instance to given pending ware.
 */
void Flag::init_ware(EditorGameBase& egbase, WareInstance& ware, PendingWare& pi) {
	last_update_ = egbase.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::init_ware (%d, %d)\n", get_position().x, get_position().y);
	}
	pi.ware = &ware;
	pi.pending = true;
	pi.nextstep = nullptr;
	pi.priority = 0;

	Transfer* trans = ware.get_transfer();
	if (trans) {
		uint32_t trans_steps = trans->get_steps_left();
		if (trans_steps < 3) {
			pi.priority = 2;
		} else if (trans_steps == 3) {
			pi.priority = 1;
		}

		Request* req = trans->get_request();
		if (req) {
			pi.priority = pi.priority + req->get_transfer_priority();
		}
	}

	ware.set_location(egbase, this);
}

/**
 * \return a ware currently waiting for a carrier to the given destflag.
 *
 * \note Due to fetch_from_flag() semantics, this function makes no sense
 * for a  building destination.
*/
Flag::PendingWare* Flag::get_ware_for_flag(Flag& destflag, bool const pending_only) {
	bool test = false;
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM get_ware_for_flag (%d, %d) -> (%d, %d) - this\n", get_position().x, get_position().y, destflag.get_position().x, destflag.get_position().y);
		test = true;
	}
	if (destflag.get_position().hash() == test_coords_hash) {
		log("NOCOM get_ware_for_flag (%d, %d) -> (%d, %d) - other\n", destflag.get_position().x, destflag.get_position().y, get_position().x, get_position().y);
		test = true;
	}
	// NOCOM test this
	if (test) log("NOCOM ware_filled_: %d\n", ware_filled_);
	for (int32_t i = 0; i < ware_filled_; ++i) {
		PendingWare* pw = &wares_[i];
		if (test) {
			log("NOCOM testing ware: %s\n", pw->ware->descr().name().c_str());
			log("NOCOM !pending_only ? %s\n", !pending_only ? "true" : "false");
			log("NOCOM pw->pending ? %s\n", pw->pending ? "true" : "false");
			log("NOCOM pw->nextstep == &destflag ? %s\n", pw->nextstep == &destflag ? "true" : "false");
			log("NOCOM destflag.allow_ware_from_flag(*pw->ware, *this) ? %s\n", destflag.allow_ware_from_flag(*pw->ware, *this) ? "true" : "false");
		}
		if ((!pending_only || pw->pending) && pw->nextstep == &destflag &&
		    destflag.allow_ware_from_flag(*pw->ware, *this)) {
			if (test) log("NOCOM found ware: %s\n", pw->ware->descr().name().c_str());
			return pw;
		}
	}

	if (test) log("NOCOM no ware found\n");
	return nullptr;
}

/**
 * Clamp the maximal value of \ref PendingWare::priority.
 * After reaching this value, the pure FIFO approach is applied
 */
#define MAX_TRANSFER_PRIORITY 16

/**
 * Called by the carriercode when the carrier is called away from his job
 * but has acknowledged a ware before. This ware is then freed again
 * to be picked by another carrier. Returns true if a ware was indeed
 * made pending again.
 */
bool Flag::cancel_pickup(Game& game, Flag& destflag) {
	last_update_ = game.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM cancel_pickup - this\n");
	}
	if (destflag.get_position().hash() == test_coords_hash) {
		log("NOCOM cancel_pickup - other\n");
	}
	// NOCOM test this
	for (int32_t i = 0; i < ware_filled_; ++i) {
		PendingWare& pw = wares_[i];
		if (!pw.pending && pw.nextstep == &destflag) {
			pw.pending = true;
			pw.ware->update(game);  //  will call call_carrier() if necessary
			return true;
		}
	}

	return false;
}

/**
 * Called by carrier code to find the best among the wares on this flag
 * that are meant for the provided dest.
 * \return index of found ware (carrier will take it)
 * or kNotFoundAppropriate (carrier will leave empty-handed)
 */
int32_t Flag::find_pending_ware(PlayerImmovable& dest) {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM find_pending_ware - this\n");
	}
	// NOCOM test this
	int32_t highest_pri = -1;
	int32_t best_index = kNotFoundAppropriate;
	bool ware_pended = false;

	for (int32_t i = 0; i < ware_filled_; ++i) {
		PendingWare& pw = wares_[i];
		if (pw.nextstep != &dest) {
			continue;
		}

		if (pw.priority < MAX_TRANSFER_PRIORITY) {
			pw.priority++;
		}
		// Release promised pickup, in case we find a preferable ware
		if (!ware_pended && !pw.pending) {
			ware_pended = pw.pending = true;
		}

		// If dest is flag, we exclude wares that can stress it
		if (&dest != building_ && !dynamic_cast<Flag&>(dest).allow_ware_from_flag(*pw.ware, *this)) {
			continue;
		}
// NOCOM we have a priority here - give it a timer too?
		if (pw.priority > highest_pri) {
			highest_pri = pw.priority;
			best_index = i;
		}
	}

	return best_index;
}

/**
 * Like find_pending_ware() above, but for carriers who have a ware to drop on this flag.
 * \return same as find_pending_ware() above, plus kDenyDrop (carrier will wait)
 */
int32_t Flag::find_swappable_ware(WareInstance& ware, Flag& destflag) {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM find_swappable_ware - this\n");
	}
	if (destflag.get_position().hash() == test_coords_hash) {
		log("NOCOM find_swappable_ware - other\n");
	}
	// NOCOM test this
	DescriptionIndex const descr_index = ware.descr_index();
	int32_t highest_pri = -1;
	int32_t best_index = kNotFoundAppropriate;
	bool has_same_ware = false;
	bool has_allowed = false;
	bool ware_pended = false;

	for (int32_t i = 0; i < ware_filled_; ++i) {
		PendingWare& pw = wares_[i];
		if (pw.nextstep != &destflag) {
			if (pw.ware->descr_index() == descr_index) {
				has_same_ware = true;
			}
			continue;
		}

		if (pw.priority < MAX_TRANSFER_PRIORITY) {
			pw.priority++;
		}
		// Release promised pickup, in case we find a preferable ware
		if (!ware_pended && !pw.pending) {
			ware_pended = pw.pending = true;
		}

		// We prefer to retrieve wares that won't stress the destflag
		if (destflag.allow_ware_from_flag(*pw.ware, *this)) {
			if (!has_allowed) {
				has_allowed = true;
				highest_pri = -1;
			}
		} else {
			if (has_allowed) {
				continue;
			}
		}

		if (pw.priority > highest_pri) {
			highest_pri = pw.priority;
			best_index = i;
		}
	}

	if (best_index > kNotFoundAppropriate) {
		return (ware_filled_ > ware_capacity_ - 3 || has_allowed) ? best_index : kNotFoundAppropriate;
	} else {
		return (ware_filled_ < ware_capacity_ - 2 ||
		        (ware_filled_ < ware_capacity_ && !has_same_ware)) ?
		          kNotFoundAppropriate :
		          kDenyDrop;
	}
}

/**
 * Called by carrier code to retrieve a ware found by the previous methods.
 */
WareInstance* Flag::fetch_pending_ware(Game& game, int32_t best_index) {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM fetch_pending_ware - this\n");
	}

	// NOCOM test this
	if (best_index < 0) {
		return nullptr;
	}
	last_update_ = game.get_gametime();

	// move the other wares up the list and return this one
	WareInstance* const ware = wares_[best_index].ware;
	--ware_filled_;
	memmove(&wares_[best_index], &wares_[best_index + 1],
	        sizeof(wares_[0]) * (ware_filled_ - best_index));

	ware->set_location(game, nullptr);
	return ware;
}

/**
 * Called by carrier code to notify waiting carriers
 * which may be interested in the new state of this flag.
 */
void Flag::ware_departing(Game& game) {
	last_update_ = game.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM ware_departing - this\n");
	}
	// NOCOM test this
	// Wake up one sleeper from the capacity queue.
	while (!capacity_wait_.empty()) {
		Worker* const w = capacity_wait_.front().get(game);
		capacity_wait_.erase(capacity_wait_.begin());
		if (w && w->wakeup_flag_capacity(game, *this)) {
			return;
		}
	}

	// Consider pending wares of neighboring flags.
	for (int32_t dir = 1; dir <= WalkingDir::LAST_DIRECTION; ++dir) {
		Road* const road = get_road(dir);
		if (!road) {
			continue;
		}

		Flag* other = &road->get_flag(Road::FlagEnd);
		if (other == this) {
			other = &road->get_flag(Road::FlagStart);
		}

		PendingWare* pw = other->get_ware_for_flag(*this, kPendingOnly);
		if (pw && road->notify_ware(game, *other)) {
			pw->pending = false;
		}
	}
}

/**
 * Accelerate potential promotion of roads adjacent to a newly promoted road.
 */
void Flag::propagate_promoted_road(Road* const promoted_road) {
	// Abort if flag has a building attached to it
	if (building_) {
		return;
	}

	// Calculate the sum of the involved wallets' adjusted value
	int32_t sum = 0;
	for (int8_t i = 0; i < WalkingDir::LAST_DIRECTION; ++i) {
		Road* const road = roads_[i];
		if (road && road != promoted_road) {
			sum += kRoadMaxWallet + road->wallet() * road->wallet();
		}
	}

	// Distribute propagation coins in a smart way
	for (int8_t i = 0; i < WalkingDir::LAST_DIRECTION; ++i) {
		Road* const road = roads_[i];
		if (road && road->get_roadtype() != RoadType::kBusy) {
			road->add_to_wallet(0.5 * (kRoadMaxWallet - road->wallet()) *
			                    (kRoadMaxWallet + road->wallet() * road->wallet()) / sum);
		}
	}
}

/**
 * Count only those wares which are awaiting to be carried along the same road.
 */
uint8_t Flag::count_wares_in_queue(PlayerImmovable& dest) const {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::count_wares_in_queue (%d, %d)\n", get_position().x, get_position().y);
	}
	uint8_t n = 0;
	for (int32_t i = 0; i < ware_filled_; ++i) {
		if (wares_[i].nextstep == &dest) {
			++n;
		}
	}
	return n;
}

/**
 * Return a List of all the wares currently on this Flag.
 * Do not rely the result value to stay valid and do not change them.
 */
Flag::Wares Flag::get_wares() {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::get_wares (%d, %d)\n", get_position().x, get_position().y);
	}
	Wares rv;

	for (int32_t i = 0; i < ware_filled_; ++i) {
		rv.push_back(wares_[i].ware);
	}

	return rv;
}

/**
 * Force a removal of the given ware from this flag.
 * Called by \ref WareInstance::cleanup()
 */
void Flag::remove_ware(EditorGameBase& egbase, WareInstance* const ware) {
	last_update_ = egbase.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::remove_ware (%d, %d)\n", get_position().x, get_position().y);
	}
	for (int32_t i = 0; i < ware_filled_; ++i) {
		if (wares_[i].ware != ware) {
			continue;
		}

		--ware_filled_;
		memmove(&wares_[i], &wares_[i + 1], sizeof(wares_[0]) * (ware_filled_ - i));

		if (upcast(Game, game, &egbase)) {
			ware_departing(*game);
		}

		return;
	}

	throw wexception("MO(%u): Flag::remove_ware: ware %u not on flag", serial(), ware->serial());
}

/**
 * If nextstep is not null, a carrier will be called to move this ware to
 * the given flag or building.
 *
 * If nextstep is null, the internal data will be reset to indicate that the
 * ware isn't going anywhere right now.
 *
 * nextstep is compared with the cached data, and a new carrier is only called
 * if that data hasn't changed.
 *
 * This behaviour is overridden by always_call_for_step_, which is set by
 * update_wares() to ensure that new carriers are called when roads are
 * split, for example.
 */
void Flag::call_carrier(Game& game, WareInstance& ware, PlayerImmovable* const nextstep) {
	last_update_ = game.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM call_carrier - this\n");
	}
	PendingWare* pi = nullptr;
	int32_t i = 0;

	// Find the PendingWare entry
	for (; i < ware_filled_; ++i) {
		if (wares_[i].ware != &ware) {
			continue;
		}

		pi = &wares_[i];
		break;
	}

	assert(pi);

	// Deal with the non-moving case quickly
	if (!nextstep) {
		pi->nextstep = nullptr;
		pi->pending = true;
		return;
	}

	// Find out whether we need to do anything
	if (pi->nextstep == nextstep && pi->nextstep != always_call_for_flag_) {
		return;  // no update needed
	}

	pi->nextstep = nextstep;
	pi->pending = false;

	// Deal with the building case
	if (nextstep == get_building()) {
		molog("Flag::call_carrier(%u): Tell building to fetch this ware\n", ware.serial());

		if (!get_building()->fetch_from_flag(game)) {
			pi->ware->cancel_moving();
			pi->ware->update(game);
		}

		return;
	}

	// Deal with the normal (flag) case
	const Flag& nextflag = dynamic_cast<const Flag&>(*nextstep);

	for (int32_t dir = 1; dir <= 6; ++dir) {
		Road* const road = get_road(dir);
		if (!road) {
			continue;
		}

		// NOCOM test this
		Flag* other = &road->get_flag(Road::FlagEnd);
		if (other == this) {
			other = &road->get_flag(Road::FlagStart);
		}
		if (other != &nextflag) {
			continue;
		}

		// Yes, this is the road we want; inform it
		// NOCOM test this
		if (other->update_ware_from_flag(game, wares_[i], *road, *this)) {
			return;
		}

		// If the road doesn't react to the ware immediately, we try other roads:
		// They might lead to the same flag!
	}

	// Nothing found, just let it be picked up by somebody
	pi->pending = true;
	return;
}

/**
 * Called by neighboring flags, before agreeing for a carrier
 * to take one of their wares heading to this flag.
 * \return true/allow on low congestion-risk.
 */
bool Flag::allow_ware_from_flag(WareInstance& ware, Flag& flag) {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM allow_ware_from_flag - this\n");
	}
	if (flag.get_position().hash() == test_coords_hash) {
		log("NOCOM allow_ware_from_flag - other\n");
	}
	// NOCOM test this
	// avoid iteration for the easy cases
	if (ware_filled_ < ware_capacity_ - 2) {
		return true;
	}

	DescriptionIndex const descr_index = ware.descr_index();
	bool has_swappable = false;
	for (int i = 0; i < ware_filled_; ++i) {
		PendingWare& pw = wares_[i];
		if (pw.pending && pw.nextstep == &flag) {
			has_swappable = true;
		} else if (pw.ware->descr_index() == descr_index) {
			return false;
		}
	}
	return ware_filled_ < ware_capacity_ || has_swappable;
}

/**
 * Called when a ware is trying to reach this flag through the provided road,
 * having just arrived to the provided flag.
 * Swaps pending wares if possible. Otherwise,
 * asks road for carrier on low congestion-risk.
 * \return false if the ware is not immediately served.
 */
bool Flag::update_ware_from_flag(Game& game, PendingWare& pw1, Road& road, Flag& flag) {
	last_update_ = game.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM update_ware_from_flag - this\n");
	}
	if (flag.get_position().hash() == test_coords_hash) {
		log("NOCOM update_ware_from_flag - other\n");
	}
	// NOCOM test this
	WareInstance& w1 = *pw1.ware;
	DescriptionIndex const w1_descr_index = w1.descr_index();
	bool has_same_ware = false;
	bool has_swappable = false;
	for (int i = 0; i < ware_filled_; ++i) {
		PendingWare& pw2 = wares_[i];
		WareInstance& w2 = *pw2.ware;
		if (w2.descr_index() == w1_descr_index) {
			if (pw2.nextstep == &flag) {
				// swap pending wares remotely
				init_ware(game, w1, pw2);
				flag.init_ware(game, w2, pw1);
				w1.update(game);
				w2.update(game);
				return true;
			}

			has_same_ware = true;
		} else if (pw2.pending && pw2.nextstep == &flag) {
			has_swappable = true;
		}
	}

	// ask road for carrier on low congestion-risk
	if (ware_filled_ < ware_capacity_ - 2 ||
	    (!has_same_ware && (ware_filled_ < ware_capacity_ || has_swappable))) {
		if (road.notify_ware(game, flag)) {
			pw1.pending = false;
			return true;
		}
	}
	return false;
}

/**
 * Called whenever a road gets broken or split.
 * Make sure all wares on this flag are rerouted if necessary.
 *
 * \note When two roads connect the same two flags, and one of these roads
 * is removed, this might cause the carrier(s) on the other road to
 * move unnecessarily. Fixing this could potentially be very expensive and
 * fragile.
 * A similar thing can happen when a road is split.
 */
void Flag::update_wares(Game& game, Flag* const other) {
	last_update_ = game.get_gametime();
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::update_wares (%d, %d)\n", get_position().x, get_position().y);
	}
	always_call_for_flag_ = other;

	for (int32_t i = 0; i < ware_filled_; ++i) {
		wares_[i].ware->update(game);
	}

	always_call_for_flag_ = nullptr;
}

bool Flag::init(EditorGameBase& egbase) {
	last_update_ = egbase.get_gametime();
	PlayerImmovable::init(egbase);

	set_position(egbase, position_);

	animstart_ = egbase.get_gametime();
	return true;
}

/**
 * Detach building and free roads.
 */
void Flag::cleanup(EditorGameBase& egbase) {
	while (!flag_jobs_.empty()) {
		delete flag_jobs_.begin()->request;
		flag_jobs_.erase(flag_jobs_.begin());
	}

	while (ware_filled_) {
		WareInstance& ware = *wares_[--ware_filled_].ware;

		ware.set_location(egbase, nullptr);
		ware.destroy(egbase);
	}

	if (building_) {
		building_->remove(egbase);  //  immediate death
		assert(!building_);
	}

	for (int8_t i = 0; i < 6; ++i) {
		if (roads_[i]) {
			roads_[i]->remove(egbase);  //  immediate death
			assert(!roads_[i]);
		}
	}

	if (Economy* e = get_economy()) {
		e->remove_flag(*this);
	}

	unset_position(egbase, position_);

	PlayerImmovable::cleanup(egbase);
}

/**
 * Destroy the building as well.
 *
 * \note This is needed in addition to the call to building_->remove() in
 * \ref Flag::cleanup(). This function is needed to ensure a fire is created
 * when a player removes a flag.
 */
void Flag::destroy(EditorGameBase& egbase) {
	if (building_) {
		building_->destroy(egbase);
		assert(!building_);
	}

	PlayerImmovable::destroy(egbase);
}

/**
 * Add a new flag job to request the worker with the given ID,
 * and to execute the given program once it's completed.
 */
void Flag::add_flag_job(Game&, DescriptionIndex const workerware, const std::string& programname) {
	if (get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::add_flag_job (%d, %d)\n", get_position().x, get_position().y);
	}
	FlagJob j;

	j.request = new Request(*this, workerware, Flag::flag_job_request_callback, wwWORKER);
	j.program = programname;

	flag_jobs_.push_back(j);
}

/**
 * This function is called when one of the flag job workers arrives on
 * the flag. Give him his job.
 */
void Flag::flag_job_request_callback(
   Game& game, Request& rq, DescriptionIndex, Worker* const w, PlayerImmovable& target) {
	Flag& flag = dynamic_cast<Flag&>(target);
	if (flag.get_position().hash() == test_coords_hash) {
		log("NOCOM **** Flag::flag_job_request_callback (%d, %d)\n", flag.get_position().x, flag.get_position().y);
	}

	assert(w);

	for (FlagJobs::iterator flag_iter = flag.flag_jobs_.begin(); flag_iter != flag.flag_jobs_.end();
	     ++flag_iter) {
		if (flag_iter->request == &rq) {
			delete &rq;

			w->start_task_program(game, flag_iter->program);

			flag.flag_jobs_.erase(flag_iter);
			return;
		}
	}

	flag.molog("BUG: flag_job_request_callback: worker not found in list\n");
}

void Flag::log_general_info(const Widelands::EditorGameBase& egbase) const {
	molog("Flag at %i,%i\n", position_.x, position_.y);

	Widelands::PlayerImmovable::log_general_info(egbase);

	if (ware_filled_) {
		molog("Wares at flag:\n");
		for (int i = 0; i < ware_filled_; ++i) {
			PendingWare& pi = wares_[i];
			molog(" %i/%i: %s(%i), nextstep %i, %s\n", i + 1, ware_capacity_,
			      pi.ware->descr().name().c_str(), pi.ware->serial(), pi.nextstep.serial(),
			      pi.pending ? "pending" : "acked by carrier");
		}
	} else {
		molog("No wares at flag.\n");
	}
}

void Flag::unfreeze_wares(Game& game) {
	const bool test = get_position().hash() == test_coords_hash;
	if (test) {
		log("NOCOM ************************************* \n");
		for (int32_t i = 0; i < ware_filled_; ++i) {
			log("Ware: %s\n", wares_[i].ware->descr().name().c_str());
		}

		for (const FlagJob& flag_job : flag_jobs_) {
			const DescriptionIndex idx = flag_job.request->get_index();
			const Coords& destination = flag_job.request->target_flag().get_position();
			std::string wareworker_name("");
			if (flag_job.request->get_type() == wwWARE) {
				wareworker_name = game.tribes().get_ware_descr(idx)->name();
			} else {
				wareworker_name = game.tribes().get_worker_descr(idx)->name();
			}
			log("Flag Job: %s %s -> (%d, %d)\n", flag_job.program.c_str(), wareworker_name.c_str(), destination.x, destination.y);
		}
	}

	const int gametime = game.get_gametime();
	if (gametime > last_update_ + kTriggerPotentiallyFrozenFlagInterval) {
		if (ware_filled_ > 0) {
			molog("%d: Potentially frozen flag for Player %d at (%d, %d) - %d wares\n", static_cast<int>(gametime / 1000), static_cast<unsigned int>(owner().player_number()), get_position().x, get_position().y, ware_filled_);
			++freeze_counter_;
		}
		last_update_ = gametime;
	} else {
		freeze_counter_ = 0;
	}
	if (freeze_counter_ > 2) {
		if ( ware_filled_ > 0) {
			log("%d: Unfreezing flag for Player %d at (%d, %d) - %d wares\n", static_cast<int>(gametime / 1000), static_cast<unsigned int>(owner().player_number()), get_position().x, get_position().y, ware_filled_);
			for (int32_t i = 0; i < ware_filled_; ++i) {
				init_ware(game, *wares_[i].ware, wares_[i]);
				wares_[i].ware->update(game);
			}
		}
		freeze_counter_ = 0;
	}
}
}
