#include "ui-download.hpp"
#include <cmath>
#include <limits>
#include <sstream>
#include "json/json.hpp"
#include "plugin.hpp"

constexpr std::string_view I18N_TITLE          = "ThemeInstaller.Title";
constexpr std::string_view I18N_STATE_WAITING  = "ThemeInstaller.State.Waiting";
constexpr std::string_view I18N_STATE_DOWNLOAD = "ThemeInstaller.State.Download";
constexpr std::string_view I18N_STATE_EXTRACT  = "ThemeInstaller.State.Extract";
constexpr std::string_view I18N_STATE_INSTALL  = "ThemeInstaller.State.Install";

constexpr std::string_view TOKEN_PATH = "<REPLACE|ME>";
constexpr std::string_view TOKEN_UUID = "<machine-token>";

void source_deleter(obs_source_t* v)
{
	obs_source_release(v);
}

void sceneitem_deleter(obs_sceneitem_t* v)
{
	obs_sceneitem_remove(v);
}

void data_deleter(obs_data_t* v)
{
	obs_data_release(v);
}

void data_item_deleter(obs_data_item_t* v)
{
	obs_data_item_release(&v);
}

void data_array_deleter(obs_data_array_t* v)
{
	obs_data_array_release(v);
}

own3d::ui::installer_thread::~installer_thread() {}

own3d::ui::installer_thread::installer_thread(std::string url, std::string name, std::filesystem::path path,
											  std::filesystem::path out_path, QObject* parent)
	: QThread(parent), _url(url), _name(name), _path(path), _out_path(out_path)
{}

void own3d::ui::installer_thread::run_download()
{
	std::ofstream             stream;
	util::curl                curl;
	own3d::ui::download_state state;

	if (own3d::testing_enabled()) {
		state = download_state::DONE;
		emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
		return;
	}

	{ // Emit signal.
		state = download_state::UNKNOWN;
		emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
	}

	{ // Set up output file.
		stream = std::ofstream(_path, std::ios::binary | std::ios::trunc | std::ios::out);
		if (stream.bad()) {
			state = download_state::FAILED;
			emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
			throw std::runtime_error("");
		}
	}

	{ // Begin curl work.
		curl.set_option(CURLOPT_HTTPGET, true);
		curl.set_option(CURLOPT_URL, _url);
		curl.set_write_callback([this, &stream](void* buf, size_t n, size_t c) {
			stream.write(reinterpret_cast<char*>(buf), n * c);
			return n * c;
		});
		curl.set_xferinfo_callback([this, &state](uint64_t total, uint64_t now, uint64_t, uint64_t) {
			if (state == download_state::UNKNOWN) {
				state = download_state::CONNECTING;
			} else if ((state == download_state::CONNECTING) && ((total != 0) || (now != 0))) {
				state = download_state::DOWNLOADING;
			}
			emit download_status(static_cast<uint64_t>(now), static_cast<uint64_t>(total), static_cast<int64_t>(state));
			return int32_t(0);
		});
		CURLcode res = curl.perform();
		if (res != CURLE_OK) {
			state = download_state::FAILED;
			emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
			throw std::runtime_error("");
		}
	}

	// Manually close the stream.
	stream.close();

	{ // Emit that we are done.
		state = download_state::DONE;
		emit download_status(uint64_t(0), uint64_t(0), static_cast<int64_t>(state));
	}
}

void own3d::ui::installer_thread::run_extract()
{
	util::zip archive{_path, _out_path};
	for (uint64_t idx = 0, edx = archive.get_file_count(); idx < edx; idx++) {
#ifdef _DEBUG
		DLOG_DEBUG("Extracting file %llu of %llu from Theme '%s'...", (idx + 1), edx, _name.c_str());
#endif
		archive.extract_file(idx, [this, &idx, &edx](uint64_t t, uint64_t n) {
#ifdef _DEBUG
			DLOG_DEBUG("  %llu of %llu bytes extracted...", n, t);
#endif
			emit extract_status(idx, edx, n, t);
		});
	}
}

static void assign_generic_source_info(obs_source_t* _child, std::shared_ptr<obs_data_t> data)
{
	// Interlacing
	obs_source_set_deinterlace_mode(
		_child, static_cast<obs_deinterlace_mode>(obs_data_get_int(data.get(), "deinterlace_mode")));
	obs_source_set_deinterlace_field_order(
		_child, static_cast<obs_deinterlace_field_order>(obs_data_get_int(data.get(), "deinterlace_field_order")));

	// Audio
	obs_source_set_balance_value(_child, obs_data_get_double(data.get(), "balance"));
	obs_source_set_audio_mixers(_child, obs_data_get_int(data.get(), "mixers"));
	obs_source_set_volume(_child, obs_data_get_double(data.get(), "volume"));
	obs_source_set_muted(_child, obs_data_get_bool(data.get(), "muted"));
	obs_source_set_sync_offset(_child, obs_data_get_int(data.get(), "sync"));
	obs_source_set_monitoring_type(_child,
								   static_cast<obs_monitoring_type>(obs_data_get_int(data.get(), "monitoring-type")));

	// Hotkeys
	obs_source_enable_push_to_mute(_child, obs_data_get_bool(data.get(), "push-to-mute"));
	obs_source_set_push_to_mute_delay(_child, obs_data_get_int(data.get(), "push-to-mute-delay"));
	obs_source_enable_push_to_talk(_child, obs_data_get_bool(data.get(), "push-to-talk"));
	obs_source_set_push_to_talk_delay(_child, obs_data_get_int(data.get(), "push-to-talk-delay"));

	// Other
	obs_source_set_enabled(_child, obs_data_get_bool(data.get(), "enabled"));
	obs_source_set_flags(_child, obs_data_get_int(data.get(), "flags"));
}

static void replace_tokens(obs_data_t* data, std::string base_directory_path)
{
	std::string_view machine_token = own3d::get_unique_identifier();
	for (obs_data_item_t* item = obs_data_first(data); item != nullptr; obs_data_item_next(&item)) {

		switch (obs_data_item_gettype(item)) {
		case obs_data_type::OBS_DATA_STRING: {
			std::string string;

			if (const char* cstr = obs_data_item_get_string(item); cstr == nullptr) {
				break;
			} else {
				string = cstr;
			}

			while (std::string::size_type pos = string.find(TOKEN_PATH)) {
				if (pos == std::string::npos)
					break;
				string = string.replace(pos, TOKEN_PATH.length(), base_directory_path);
			}
			while (std::string::size_type pos = string.find(TOKEN_UUID)) {
				if (pos == std::string::npos)
					break;
				string = string.replace(pos, TOKEN_UUID.length(), machine_token);
			}


			obs_data_item_set_string(&item, string.c_str());
			break;
		}
		case obs_data_type::OBS_DATA_OBJECT: {
			obs_data_t* chld = obs_data_item_get_obj(item);
			replace_tokens(chld, base_directory_path);
		}
		}
	}
}

static void parse_filters_for_source(obs_source_t* source, std::shared_ptr<obs_data_array_t> filters)
{
	for (size_t idx = 0, edx = obs_data_array_count(filters.get()); idx < edx; idx++) {
		// Filters are basically a list of sources that belong to a certain source and are unique to that source.
		// Their data is identical to any other source.
		std::shared_ptr<obs_data_t> item{obs_data_array_item(filters.get(), idx), data_deleter};

		// Skip any source that has no name or an invalid name.
		const char* name = obs_data_get_string(item.get(), "name");
		if ((!name) || (strlen(name) <= 0))
			continue;

		// If it has a name, let's try to create it.
		std::string                 id = obs_data_get_string(item.get(), "id");
		std::shared_ptr<obs_data_t> settings{obs_data_get_obj(item.get(), "settings"), data_deleter};

		std::shared_ptr<obs_source_t> filter{obs_source_create_private(id.c_str(), name, settings.get()),
											 source_deleter};
		if (!filter) {
			// If creation of the source failed, skip the source.
			continue;
		}

		// Assign generic data.
		assign_generic_source_info(filter.get(), item);

		// Attach to parent source.
		obs_source_filter_add(source, filter.get());

		// Load filter.
		obs_source_load(filter.get());
	}
}

static void parse_source(std::shared_ptr<obs_data_t> item, std::string path)
{
	const char* name = obs_data_get_string(item.get(), "name");
	const char* id   = obs_data_get_string(item.get(), "id");

	if ((!name || (strlen(name) == 0)) || (!id || (strlen(id) == 0))) {
		return;
	}

	if ((strcmp(id, "scene") == 0) || (strcmp(id, "group") == 0)) {
		return;
	}

	std::shared_ptr<obs_data_t> settings{obs_data_get_obj(item.get(), "settings"),
										 [](obs_data_t* v) { obs_data_release(v); }};

	// Need to convert all relative paths to proper paths.
	replace_tokens(settings.get(), path);

	// Create the source
	obs_source_t* source = obs_source_create(id, name, settings.get(), nullptr);
	assign_generic_source_info(source, item);

	// But wait, there's more! Filters.
	parse_filters_for_source(
		source, std::shared_ptr<obs_data_array_t>{obs_data_get_array(item.get(), "filters"), data_array_deleter});
}

static void parse_scene(std::shared_ptr<obs_data_t> item, std::string path)
{
	const char* name = obs_data_get_string(item.get(), "name");
	const char* id   = obs_data_get_string(item.get(), "id");

	if ((!name || (strlen(name) == 0)) || (!id || (strlen(id) == 0))) {
		return;
	}

	if (strcmp(id, "scene") != 0) {
		return;
	}

	std::shared_ptr<obs_data_t> settings{obs_data_get_obj(item.get(), "settings"),
										 [](obs_data_t* v) { obs_data_release(v); }};

	// Need to convert all relative paths to proper paths.
	replace_tokens(settings.get(), path);

	// Create the source
	obs_scene_t*  scene  = obs_scene_create(name);
	obs_source_t* source = obs_scene_get_source(scene);
	assign_generic_source_info(source, item);

	// But wait, there's more! Filters.
	parse_filters_for_source(
		source, std::shared_ptr<obs_data_array_t>{obs_data_get_array(item.get(), "filters"), data_array_deleter});

	// And now we add the actual items.
	std::shared_ptr<obs_data_array_t> items{obs_data_get_array(settings.get(), "items"), data_array_deleter};
	for (size_t idx = 0, edx = obs_data_array_count(items.get()); idx < edx; idx++) {
		vec2               pos;
		vec2               scale;
		vec2               bounds;
		obs_sceneitem_crop crop;

		std::shared_ptr<obs_data_t> obj{obs_data_array_item(items.get(), idx), data_deleter};
		if (!obj) {
			continue;
		}

		const char* name = obs_data_get_string(obj.get(), "name");
		if (!name) {
			continue;
		}

		auto _child = obs_get_source_by_name(name);
		if (!_child) {
			continue;
		}

		obs_sceneitem_t* item = obs_scene_add(scene, _child);
		if (!item) {
			continue;
		}

		obs_sceneitem_set_alignment(item, obs_data_get_int(obj.get(), "align"));
		obs_data_get_vec2(obj.get(), "bounds", &bounds);
		obs_sceneitem_set_bounds(item, &bounds);
		obs_sceneitem_set_bounds_alignment(item, obs_data_get_int(obj.get(), "bounds_align"));
		obs_sceneitem_set_bounds_type(item, static_cast<obs_bounds_type>(obs_data_get_int(obj.get(), "bounds_type")));
		crop.left   = obs_data_get_int(obj.get(), "crop_left");
		crop.right  = obs_data_get_int(obj.get(), "crop_right");
		crop.top    = obs_data_get_int(obj.get(), "crop_top");
		crop.bottom = obs_data_get_int(obj.get(), "crop_bottom");
		obs_sceneitem_set_crop(item, &crop);
		obs_sceneitem_set_locked(item, obs_data_get_bool(obj.get(), "locked"));
		obs_data_get_vec2(obj.get(), "pos", &pos);
		obs_sceneitem_set_pos(item, &pos);
		obs_sceneitem_set_rot(item, obs_data_get_double(obj.get(), "rot"));
		obs_data_get_vec2(obj.get(), "scale", &scale);
		obs_sceneitem_set_scale(item, &scale);
		obs_sceneitem_set_scale_filter(item, static_cast<obs_scale_type>(obs_data_get_int(obj.get(), "scale_filter")));
		obs_sceneitem_set_visible(item, obs_data_get_bool(obj.get(), "visible"));
		obs_sceneitem_force_update_transform(item);
	}

	obs_frontend_set_current_scene(source);
	obs_frontend_set_current_preview_scene(source);
}

static void parse_transition(std::shared_ptr<obs_data_t> item, std::string path)
{
	const char* name = obs_data_get_string(item.get(), "name");
	const char* id   = obs_data_get_string(item.get(), "id");

	if ((!name || (strlen(name) == 0)) || (!id || (strlen(id) == 0))) {
		return;
	}

	std::shared_ptr<obs_data_t> settings{obs_data_get_obj(item.get(), "settings"),
										 [](obs_data_t* v) { obs_data_release(v); }};

	// Need to convert all relative paths to proper paths.
	replace_tokens(settings.get(), path);

	// Create the transition.
	obs_source_t* source = obs_source_create(id, name, settings.get(), nullptr);

	//obs_frontend_set_current_transition(source);
}

void own3d::ui::installer_thread::run_install()
{
	std::shared_ptr<obs_data_t> data;

	emit install_status(false);

	{ // Load the file.
		std::filesystem::path data_path = _out_path;
		data_path.append("data.json");
		if (!std::filesystem::exists(data_path)) {
			throw std::runtime_error("data.json missing");
		}

		std::string data_path_str = data_path.string();
		data = std::shared_ptr<obs_data_t>(obs_data_create_from_json_file(data_path_str.c_str()), obs_data_deleter);
	}

	// Find a valid scene collection name.
	std::string collection_name = _name;
	{
		char** collections = obs_frontend_get_scene_collections();
		bool   found_name  = false;
		for (size_t n = 0; !found_name; n++) {
			found_name = true;
			for (char** ptr = collections; *ptr != nullptr; ptr++) {
				if (collection_name == (*ptr)) {
					found_name = false;
					break;
				}
			}
			if (!found_name) {
				std::stringstream sstr;
				sstr << _name << " " << (n + 1);
				collection_name = sstr.str();
			}
		}
	}

	// Create a collection.
	if (!obs_frontend_add_scene_collection(collection_name.c_str())) {
		throw std::runtime_error("Failed to create a scene collection.");
	}

	// Begin with importing.
	obs_source_t* default_scene;
	{ // First let's rename the default scene to something completely nonsensical.
		obs_enum_scenes(
			[](void* ptr, obs_source_t* src) {
				*reinterpret_cast<obs_source_t**>(ptr) = src;
				return false;
			},
			&default_scene);
		if (default_scene != nullptr) {
			obs_source_set_name(default_scene, "OWN3D.TV Scene Import Successful - delete this Scene");
		}
	}

	{
		std::shared_ptr<obs_data_array_t> darr =
			std::shared_ptr<obs_data_array_t>(obs_data_get_array(data.get(), "sources"), data_array_deleter);

		// Create all sources first which are neither scenes or groups.
		for (size_t idx = 0, edx = obs_data_array_count(darr.get()); idx < edx; idx++) {
			auto item = std::shared_ptr<obs_data_t>{obs_data_array_item(darr.get(), idx), data_deleter};
			parse_source(item, _out_path.string());
		}

		// Then create all scenes (NOT groups).
		for (size_t idx = 0, edx = obs_data_array_count(darr.get()); idx < edx; idx++) {
			auto item = std::shared_ptr<obs_data_t>{obs_data_array_item(darr.get(), idx), data_deleter};
			parse_scene(item, _out_path.string());
		}

		// Trigger the load hook on all sources but not scenes or groups.
		obs_enum_sources(
			[](void*, obs_source_t* src) {
				obs_source_load(src);
				return false;
			},
			nullptr);
	}

	{ // Create all transitions.
		std::shared_ptr<obs_data_array_t> darr =
			std::shared_ptr<obs_data_array_t>(obs_data_get_array(data.get(), "transitions"), data_array_deleter);

		// Create all sources first which are neither scenes or groups.
		for (size_t idx = 0, edx = obs_data_array_count(darr.get()); idx < edx; idx++) {
			auto item = std::shared_ptr<obs_data_t>{obs_data_array_item(darr.get(), idx), data_deleter};
			parse_transition(item, _out_path.string());
		}
	}

	// Release default scene.
	obs_source_remove(default_scene);

	emit install_status(true);
	obs_frontend_save();
}

void own3d::ui::installer_thread::run()
try {
	run_download();
	run_extract();
	run_install();
} catch (std::exception const& ex) {
	DLOG_ERROR("Installation of Theme '%s' failed due to error: %s", _name.c_str(), ex.what());
	emit error();
} catch (...) {
	emit error();
}

own3d::ui::installer::~installer() {}

own3d::ui::installer::installer(const QUrl& url, const QString& name, const QString& hash)
	: QDialog(reinterpret_cast<QWidget*>(obs_frontend_get_main_window())), Ui::ThemeDownload(), _download_url(url),
	  _theme_name(name), _download_hash(hash)
{
	// Check if we are in test mode or now.
	if (!own3d::testing_enabled()) {
		// Generate a download filename.
		_theme_archive_path = std::filesystem::temp_directory_path();
		_theme_archive_path.append("own3d");
		std::filesystem::create_directories(_theme_archive_path);
		_theme_archive_path.append(name.toStdString());
		if (hash.length() > 0)
			_theme_archive_path.concat(hash.toStdString().substr(0, 16));
		_theme_archive_path.concat(".pack");
	} else {
		_theme_archive_path = own3d::testing_archive_path();
		_theme_name         = QString::fromUtf8(own3d::testing_archive_name().data());
	}

	{ // Generate a theme extraction path
		{
			char* buf   = obs_module_config_path("themes");
			_theme_path = buf;
			bfree(buf);
		}
		_theme_path.append(_theme_name.toStdString());
		std::filesystem::create_directories(_theme_path);
	}

	{ // Update UI elements.
		setupUi(this);
		setModal(true);
		buttonBox->setVisible(false);
		show();
		{
			std::vector<char> buffer(2048);
			snprintf(buffer.data(), buffer.size(), D_TRANSLATE(I18N_TITLE.data()), _theme_name.toStdString().c_str());
			setWindowTitle(QString::fromStdString(std::string{buffer.data()}));
		}
		themeName->setText(_theme_name);
	}

	// Spawn a worker thread and begin work.
	_worker = new installer_thread(url.toString().toStdString(), _theme_name.toStdString(), _theme_archive_path,
								   _theme_path, this);
	connect(_worker, &own3d::ui::installer_thread::download_status, this, &own3d::ui::installer::handle_download_status,
			Qt::QueuedConnection);
	connect(_worker, &own3d::ui::installer_thread::extract_status, this, &own3d::ui::installer::handle_extract_status,
			Qt::QueuedConnection);
	connect(_worker, &own3d::ui::installer_thread::install_status, this, &own3d::ui::installer::handle_install_status,
			Qt::QueuedConnection);
	_worker->start();
}

void own3d::ui::installer::update_progress(double_t percent, bool is_download, bool is_extract, bool is_install)
{
	if (is_download) {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_DOWNLOAD.data()))));
		percent /= 3.;
	} else if (is_extract) {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_EXTRACT.data()))));
		percent /= 3.;
		percent += (1. / 3.);
	} else if (is_install) {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_INSTALL.data()))));
		percent /= 3.;
		percent += (2. / 3.);
	} else {
		status->setText(QString::fromStdString(std::string(D_TRANSLATE(I18N_STATE_WAITING.data()))));
		percent = NAN;
	}

	if (isnan(percent)) {
		progressBar->setRange(0, 0);
		progressBar->setValue(0);
		progressBar->setTextVisible(false);
	} else {
		uint32_t val = static_cast<uint32_t>(percent * static_cast<double_t>((std::numeric_limits<uint32_t>::max)()));
		progressBar->setRange((std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)());
		progressBar->setValue(static_cast<int32_t>(static_cast<int64_t>((std::numeric_limits<int32_t>::min)()) + val));
		progressBar->setTextVisible(true);
	}
}

void own3d::ui::installer::handle_download_status(uint64_t now_bytes, uint64_t total_bytes, int64_t state)
{
	switch (static_cast<own3d::ui::download_state>(state)) {
	case download_state::UNKNOWN:
	case download_state::CONNECTING:
		DLOG_DEBUG("Starting download of Theme '%s'...", _theme_name.toStdString().c_str());
		update_progress(0, false, false, false);
		break;
	case download_state::DOWNLOADING:
		DLOG_DEBUG("Download of Theme '%s' at %llu/%llu...", _theme_name.toStdString().c_str(), now_bytes, total_bytes);
		update_progress(static_cast<double_t>(now_bytes) / static_cast<double_t>(total_bytes), true, false, false);
		break;
	case download_state::DONE:
		DLOG_DEBUG("Download of Theme '%s' complete.", _theme_name.toStdString().c_str());
		update_progress(1.0, true, false, false);
		break;
	case download_state::FAILED:
		DLOG_DEBUG("Download of Theme '%s' failed.", _theme_name.toStdString().c_str());
		update_progress(NAN, true, false, false);
		emit error();
		break;
	}
}

void own3d::ui::installer::handle_extract_status(uint64_t now_files, uint64_t total_files, uint64_t now_bytes,
												 uint64_t total_bytes)
{
	double_t progress  = static_cast<double_t>(now_bytes) / static_cast<double_t>(total_bytes);
	double_t progress2 = (static_cast<double_t>(now_files) + progress) / static_cast<double_t>(total_files);
	update_progress(progress2, false, true, false);
}

void own3d::ui::installer::handle_install_status(bool complete)
{
	update_progress(complete ? 1.0 : 0.0, false, false, true);
}
