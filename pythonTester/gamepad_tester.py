"""
Testeur de manette — affiche axes et boutons en temps réel.
Lancer avec : py -3.11 gamepad_tester.py
"""
import sys
import time

try:
    import pygame
except ImportError:
    print("pygame non installé. Lancer : py -3.11 -m pip install pygame")
    sys.exit(1)


def main():
    pygame.init()
    pygame.joystick.init()

    count = pygame.joystick.get_count()
    if count == 0:
        print("Aucune manette détectée.")
        sys.exit(1)

    print(f"{count} manette(s) détectée(s) :")
    for i in range(count):
        print(f"  [{i}] {pygame.joystick.Joystick(i).get_name()}")

    if count == 1:
        idx = 0
    else:
        idx = int(input("Choisir l'index : "))

    joy = pygame.joystick.Joystick(idx)
    joy.init()
    print(f"\nManette : {joy.get_name()}")
    print(f"  {joy.get_numaxes()} axes  |  {joy.get_numbuttons()} boutons  |  {joy.get_numhats()} hats")
    print("\nCtrl+C pour quitter\n")

    try:
        while True:
            pygame.event.pump()

            axes = [f"A{i}:{joy.get_axis(i):+.2f}" for i in range(joy.get_numaxes())]
            buttons = [str(i) for i in range(joy.get_numbuttons()) if joy.get_button(i)]
            hats = [f"H{i}:{joy.get_hat(i)}" for i in range(joy.get_numhats())]

            parts = "  ".join(axes)
            if buttons:
                parts += f"  BTN[{','.join(buttons)}]"
            if hats:
                parts += f"  {' '.join(hats)}"

            print(f"\r{parts:<80}", end="", flush=True)
            time.sleep(0.05)

    except KeyboardInterrupt:
        print("\nArrêt.")


if __name__ == "__main__":
    main()
