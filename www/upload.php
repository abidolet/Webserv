<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <title>Upload de fichier</title>
</head>
<body>
    <h2>Envoyer un fichier</h2>
    <form action="" method="post" enctype="multipart/form-data">
        <input type="file" name="fileToUpload" id="fileToUpload">
        <input type="submit" value="Envoyer" name="submit">
    </form>

    <?php
    if ($_SERVER["REQUEST_METHOD"] == "POST" && isset($_FILES["fileToUpload"])) {
        $targetDir = "uploads/"; // Dossier où les fichiers seront stockés
        $targetFile = $targetDir . basename($_FILES["fileToUpload"]["name"]);
        $uploadOk = 1;
        $fileType = strtolower(pathinfo($targetFile, PATHINFO_EXTENSION));

        // Vérifier si le dossier uploads existe, sinon le créer
        if (!file_exists($targetDir)) {
            mkdir($targetDir, 0777, true);
        }

        // Vérifier la taille du fichier (ex. : max 5MB)
        if ($_FILES["fileToUpload"]["size"] > 5000000) {
            echo "Erreur : le fichier est trop volumineux.";
            $uploadOk = 0;
        }

        // Limiter les types de fichiers autorisés (ex. : jpg, png, pdf)
        $allowedTypes = array("jpg", "png", "jpeg", "pdf");
        if (!in_array($fileType, $allowedTypes)) {
            echo "Erreur : seuls les fichiers JPG, JPEG, PNG et PDF sont autorisés.";
            $uploadOk = 0;
        }

        // Vérifier si le fichier existe déjà
        if (file_exists($targetFile)) {
            echo "Erreur : le fichier existe déjà.";
            $uploadOk = 0;
        }

        // Vérifier les erreurs d'upload
        if ($_FILES["fileToUpload"]["error"] !== UPLOAD_ERR_OK) {
            echo "Erreur lors de l'upload du fichier.";
            $uploadOk = 0;
        }

        // Si tout est OK, déplacer le fichier
        if ($uploadOk == 1) {
            if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $targetFile)) {
                echo "Le fichier ". htmlspecialchars(basename($_FILES["fileToUpload"]["name"])) . " a été envoyé avec succès.";
            } else {
                echo "Erreur : impossible d'envoyer le fichier.";
            }
        }
    }
    ?>
</body>
</html>
