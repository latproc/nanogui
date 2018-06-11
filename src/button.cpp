/*
    src/button.cpp -- [Normal/Toggle/Radio/Popup] Button widget

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/button.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/serializer/core.h>
#include <iostream>

NAMESPACE_BEGIN(nanogui)

Button::Button(Widget *parent, const std::string &caption, int icon)
    : Widget(parent), mCaption(caption), mIcon(icon),
      mIconPosition(IconPosition::LeftCentered), mPushed(false),
      mFlags(NormalButton), mBackgroundColor(Color(0, 0)),
      mTextColor(Color(0, 0)) { }

Vector2i Button::preferredSize(NVGcontext *ctx) const {
    int fontSize = mFontSize == -1 ? mTheme->mButtonFontSize : mFontSize;
    nvgFontSize(ctx, fontSize);
    nvgFontFace(ctx, "sans-bold");
    float tw = nvgTextBounds(ctx, 0,0, mCaption.c_str(), nullptr, nullptr);
    float iw = 0.0f, ih = fontSize;

    if (mIcon) {
        if (nvgIsFontIcon(mIcon)) {
            ih *= icon_scale();
            nvgFontFace(ctx, "icons");
            nvgFontSize(ctx, ih);
            iw = nvgTextBounds(ctx, 0, 0, utf8(mIcon).data(), nullptr, nullptr)
                + mSize.y() * 0.15f;
        } else {
            int w, h;
            ih *= 0.9f;
            nvgImageSize(ctx, mIcon, &w, &h);
            iw = w * ih / h;
        }
    }
    return Vector2i((int)(tw + iw) + 20, fontSize + 10);
}

bool Button::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    Widget::mouseButtonEvent(p, button, down, modifiers);
    /* Temporarily increase the reference count of the button in case the
       button causes the parent window to be destructed */
    ref<Button> self = this;

    if (button == GLFW_MOUSE_BUTTON_1 && mEnabled) {
        bool pushedBackup = mPushed;
        //std::cout << "button push state was " << mPushed << "'\n";
        if (down) {
            //std::cout << "button mouse down\n";
            if (mFlags & RadioButton) {
                if (mButtonGroup.empty()) {
                  //std::cout << "button radio empty group\n";

                    for (auto widget : parent()->children()) {
                        Button *b = dynamic_cast<Button *>(widget);
                        if (b != this && b && (b->flags() & RadioButton) && b->mPushed) {
                          //std::cout << "button unpushing button " << b->caption() << "\n";

                            b->mPushed = false;
                            if (b->mChangeCallback) {
                                //std::cout << "button calling unpushed button change(false)\n";
                                b->mChangeCallback(false);
                            }
                        }
                    }
                } else {
                  //std::cout << "button radio in group\n";
                    for (auto b : mButtonGroup) {
                        if (b != this && (b->flags() & RadioButton) && b->mPushed) {
                          //std::cout << "button unpushing button " << b->caption() << "\n";

                            b->mPushed = false;
                            if (b->mChangeCallback) {
                                //std::cout << "button calling unpushed button change(false)\n";
                                b->mChangeCallback(false);
                              }
                        }
                    }
                }
            }
            if (mFlags & PopupButton) {
              //std::cout << "button popup button\n";

                for (auto widget : parent()->children()) {
                    Button *b = dynamic_cast<Button *>(widget);
                    if (b != this && b && (b->flags() & PopupButton) && b->mPushed) {
                      //std::cout << "button unpushing button " << b->caption() << "\n";

                        b->mPushed = false;
                        if (b->mChangeCallback) {
                            //std::cout << "button calling unpushed button change(false)\n";
                            b->mChangeCallback(false);
                          }
                    }
                }
            }
            if (mFlags & SetOnButton || mFlags & SetOffButton) {
                if (!mPushed) {
                    mPushed = true;
                    //std::cout << "set on/off button is now pushed\n";
                }
            }
            else {
                if (mFlags & ToggleButton) {
                    mPushed = !mPushed;
                    //std::cout << "button toggled to " << mPushed << "\n";
                }
                else {
                    mPushed = true;
                    //std::cout << "button is now pushed\n";
                }
               if (mFlags & RemoteButton && mCallback) {
                    //std::cout << "button calling its callback\n";
                    mCallback();
                }
            }
        } else if (mPushed) {
            //std::cout << "button mouse up when pushed\n";
            if (contains(p) && mCallback && !(mFlags & RemoteButton)) {
                //std::cout << "button calling its callback\n";
                mCallback();
            }
            if (mFlags & NormalButton) {
              //std::cout << "button (normal) resetting state to not pushed\n";
                mPushed = false;
            }
            else if (mFlags == 0) {
                mPushed = false;
            }
        }
        if (pushedBackup != mPushed && mChangeCallback && !(mFlags & RemoteButton)) {
            //std::cout << "button changed state so calling change callback " << mPushed << "\n";
            if (mFlags & SetOffButton)
                mChangeCallback(false);
            else
                mChangeCallback(mPushed);
        }
        if (mFlags & RemoteButton)
            mPushed = pushedBackup;
        return true;
    }
    return false;
}

void Button::draw(NVGcontext *ctx) {
    Widget::draw(ctx);

    NVGcolor gradTop = mTheme->mButtonGradientTopUnfocused;
    NVGcolor gradBot = mTheme->mButtonGradientBotUnfocused;

    if (mPushed) {
        gradTop = mTheme->mButtonGradientTopPushed;
        gradBot = mTheme->mButtonGradientBotPushed;
    } else if (mMouseFocus && mEnabled) {
        gradTop = mTheme->mButtonGradientTopFocused;
        gradBot = mTheme->mButtonGradientBotFocused;
    }

    nvgBeginPath(ctx);

    nvgRoundedRect(ctx, mPos.x() + 1, mPos.y() + 1.0f, mSize.x() - 2,
                   mSize.y() - 2, mTheme->mButtonCornerRadius - 1);

    if (mBackgroundColor.w() != 0) {
        nvgFillColor(ctx, Color(mBackgroundColor.head<3>(), 1.f));
        nvgFill(ctx);
        if (mPushed) {
            gradTop.a = gradBot.a = 0.8f;
        } else {
            double v = 1 - mBackgroundColor.w();
            gradTop.a = gradBot.a = mEnabled ? v : v * .5f + .5f;
        }
    }

    NVGpaint bg = nvgLinearGradient(ctx, mPos.x(), mPos.y(), mPos.x(),
                                    mPos.y() + mSize.y(), gradTop, gradBot);

    nvgFillPaint(ctx, bg);
    nvgFill(ctx);

    nvgBeginPath(ctx);
    nvgStrokeWidth(ctx, 1.0f);
    nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + (mPushed ? 0.5f : 1.5f), mSize.x() - 1,
                   mSize.y() - 1 - (mPushed ? 0.0f : 1.0f), mTheme->mButtonCornerRadius);
    nvgStrokeColor(ctx, mTheme->mBorderLight);
    nvgStroke(ctx);

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, mSize.x() - 1,
                   mSize.y() - 2, mTheme->mButtonCornerRadius);
    nvgStrokeColor(ctx, mTheme->mBorderDark);
    nvgStroke(ctx);

    int fontSize = mFontSize == -1 ? mTheme->mButtonFontSize : mFontSize;
    nvgFontSize(ctx, fontSize);
    nvgFontFace(ctx, "sans-bold");
    float tw = nvgTextBounds(ctx, 0,0, mCaption.c_str(), nullptr, nullptr);

    Vector2f center = mPos.cast<float>() + mSize.cast<float>() * 0.5f;
    Vector2f textPos(center.x() - tw * 0.5f, center.y() - 1);
    NVGcolor textColor =
        mTextColor.w() == 0 ? mTheme->mTextColor : mTextColor;
    if (!mEnabled)
        textColor = mTheme->mDisabledTextColor;

    if (mIcon) {
        auto icon = utf8(mIcon);

        float iw, ih = fontSize;
        if (nvgIsFontIcon(mIcon)) {
            //ih *= 1.5f;
            //if (mIconPosition == IconPosition::Filled) { ih=mSize.y(); }
            ih *= icon_scale(); // updated nanogui
            nvgFontSize(ctx, ih);
            nvgFontFace(ctx, "icons");
            iw = nvgTextBounds(ctx, 0, 0, icon.data(), nullptr, nullptr);
        } else {
            int w, h;
            ih *= 0.9f;
            if (mIconPosition == IconPosition::Filled) { ih = mSize.y(); }
            nvgImageSize(ctx, mIcon, &w, &h);
            iw = w * ih / h;
        }
        if (mIconPosition != IconPosition::Filled && mCaption != "")
            iw += mSize.y() * 0.15f;
        nvgFillColor(ctx, textColor);
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        Vector2f iconPos = center;
        iconPos.y() -= 1;

        if (mIconPosition == IconPosition::LeftCentered) {
            iconPos.x() -= (tw + iw) * 0.5f;
            textPos.x() += iw * 0.5f;
        } else if (mIconPosition == IconPosition::RightCentered) {
            textPos.x() -= iw * 0.5f;
            iconPos.x() += tw * 0.5f;
        } else if (mIconPosition == IconPosition::Left) {
            iconPos.x() = mPos.x() + 8;
        } else if (mIconPosition == IconPosition::Right) {
            iconPos.x() = mPos.x() + mSize.x() - iw - 8;
        }
        else if (mIconPosition == IconPosition::Filled) {
            iconPos.x() = mPos.x(); iconPos.y() = mPos.y() + ih/2; // compensates for offset below
        }

        if (nvgIsFontIcon(mIcon)) {
            nvgText(ctx, iconPos.x(), iconPos.y()+1, icon.data(), nullptr);
        } else {
            NVGpaint imgPaint = nvgImagePattern(ctx,
                    iconPos.x(), iconPos.y() - ih/2, iw, ih, 0, mIcon, mEnabled ? 0.5f : 0.25f);

            nvgFillPaint(ctx, imgPaint);
            nvgFill(ctx);
        }
    }

    nvgFontSize(ctx, fontSize);
    nvgFontFace(ctx, "sans-bold");
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, mTheme->mTextColorShadow);
    nvgText(ctx, textPos.x(), textPos.y(), mCaption.c_str(), nullptr);
    nvgFillColor(ctx, textColor);
    nvgText(ctx, textPos.x(), textPos.y() + 1, mCaption.c_str(), nullptr);
}

void Button::save(Serializer &s) const {
    Widget::save(s);
    s.set("caption", mCaption);
    s.set("icon", mIcon);
    s.set("iconPosition", (int) mIconPosition);
    s.set("pushed", mPushed);
    s.set("flags", mFlags);
    s.set("backgroundColor", mBackgroundColor);
    s.set("textColor", mTextColor);
}

bool Button::load(Serializer &s) {
    if (!Widget::load(s)) return false;
    if (!s.get("caption", mCaption)) return false;
    if (!s.get("icon", mIcon)) return false;
    if (!s.get("iconPosition", mIconPosition)) return false;
    if (!s.get("pushed", mPushed)) return false;
    if (!s.get("flags", mFlags)) return false;
    if (!s.get("backgroundColor", mBackgroundColor)) return false;
    if (!s.get("textColor", mTextColor)) return false;
    return true;
}

NAMESPACE_END(nanogui)
