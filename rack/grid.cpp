#include "grid.hpp"
#include "virtual_gpio.h"

// cheat for now by accessing the module's LED buffer directly
extern uint8_t monomeLedBuffer[256];


template<size_t GRID_X, size_t GRID_Y>
struct Grid : Module {

    enum ParamIds {
        NUM_PARAMS = GRID_X * GRID_Y
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS = GRID_X * GRID_Y
    };

    const int X = GRID_X;
    const int Y = GRID_Y;
    bool pressedState[GRID_X * GRID_Y];

    Grid() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) 
    {
        simulate_monome_connect();
    }

    void step() override {
        for (size_t i = 0; i < GRID_X; i++)
        {
            for (size_t j = 0; j < GRID_Y; j++)
            {
                int n = i | (j << 4);
                if (params[n].value != pressedState[n])
                {
                    simulate_monome_key(i, j, params[n].value);
                    pressedState[n] = params[n].value;
                }
            }
        }
    }

    json_t *toJson() override {
        json_t *rootJ = json_object();
        return rootJ;
    }

    void fromJson(json_t *rootJ) override {
    }

    void reset() override {
    }

    void randomize() override {
    }
};


struct MonomePad : ParamWidget {

    uint8_t* ledAddress;
    int key_x;
    int key_y;

    MonomePad() : ParamWidget() {
    }

    Grid128Widget* getGrid() {
        return dynamic_cast<Grid128Widget*>(parent);
    }
        
    void setKeyAddress(int x, int y, uint8_t* ledByte)
    {
        ledAddress = ledByte;
        key_x = x;
        key_y = y;
    }

    void draw(NVGcontext *vg) override
    {
        uint8_t val = *ledAddress;
        NVGcolor color1 = nvgRGB(val * 14 + 31, val * 13 + 31, val * 11 + 16);
        NVGcolor color2 = nvgRGB(val * 12 + 31, val * 11 + 15, val * 10 + 8);

        nvgBeginPath(vg);
        auto paint = nvgBoxGradient(vg, 0, 0, box.size.x, box.size.y, box.size.x * 0.5, box.size.x * 1.2, color1, color2);
        nvgRoundedRect(vg, 0, 0, box.size.x, box.size.y, 4);
        if (val > 0)
        {
            nvgFillPaint(vg, paint);
        } 
        else
        {
            nvgFillColor(vg, nvgRGB(0,0,0));
        }
        nvgFill(vg);
    }

    void onMouseDown(EventMouseDown &e) override
    {
        e.target = this;
        e.consumed = true;
        setValue(1.0);
    }

    void onMouseUp(EventMouseUp &e) override
    {
        e.target = this;
        e.consumed = true;
        setValue(0.0);
    }

    void onDragStart(EventDragStart &e) override
    {
        getGrid()->padsTouchedThisDrag.clear();
        getGrid()->padsTouchedThisDrag.insert(this);
    }

    void onDragEnd(EventDragEnd &e) override
    {
        setValue(0.0);
        getGrid()->padsTouchedThisDrag.clear();
    }

    void onDragMove(EventDragMove &e) override
    {
    }

    void onDragLeave(EventDragEnter &e) override
    {
        setValue(0.0);
    }

    void onDragEnter(EventDragEnter &e) override
    {
        auto ret = getGrid()->padsTouchedThisDrag.insert(this);
        if (ret.second) 
        {
            // this button wasn't already in the touched set
            setValue(1.0);
        }
    }
};


Grid128Widget::Grid128Widget() {

    auto *module = new Grid<16, 8>();
    setModule(module);
    box.size = Vec(15*48, 380);

    {
        //auto panel = new SVGPanel();
        auto panel = new LightPanel();
        panel->box.size = box.size;
        //panel->setBackground(SVG::load(assetPlugin(plugin, "res/whitewhale.svg")));
        addChild(panel);
    }

    Vec margins(20, 20);
    int spacing = 9;

    int max_width = (box.size.x - margins.x * 2 - (module->X - 1) * spacing) / module->X;
    int max_height = (box.size.y - margins.y * 2 - (module->Y - 1) * spacing) / module->Y;
    int button_size = max_width > max_height ? max_width : max_height;

    for(int i = 0; i < module->X; i++) 
    {
        for(int j = 0; j < module->Y; j++)
        {
            int x = margins.x + i * (button_size + spacing);
            int y = margins.y + j * (button_size + spacing);
            int n = i | (j << 4);

            MonomePad* pad = (MonomePad*)createParam<MonomePad>(Vec(x, y), module, n, 0, 1.0, 0);
            pad->setKeyAddress(x, y, monomeLedBuffer + n);
            pad->box.size = Vec(button_size, button_size);
            addParam(pad);
        }
    }
}

json_t *Grid128Widget::toJson() {
	json_t *rootJ = json_object();

	// manufacturer
	json_object_set_new(rootJ, "plugin", json_string(model->plugin->slug.c_str()));
	// model
	json_object_set_new(rootJ, "model", json_string(model->slug.c_str()));
	// pos
	json_t *posJ = json_pack("[f, f]", (double) box.pos.x, (double) box.pos.y);
	json_object_set_new(rootJ, "pos", posJ);
	
	return rootJ;
}

void Grid128Widget::fromJson(json_t *rootJ) {
	// pos
	json_t *posJ = json_object_get(rootJ, "pos");
	double x, y;
	json_unpack(posJ, "[F, F]", &x, &y);
	box.pos = Vec(x, y);
}