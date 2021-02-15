# Structure du projet

# racine du projet

On y met tous les fichiers utiles: l'editorconfig, le jenkinsfile et les master-playbooks. Un master-playbook est un playbook qui appelle d'autres playbooks. Cela permet une structure avec un niveau supplémentaire et d'avoir un regroupement de playbook.

## Inventory

En général, un inventory est un dossier représentant un groupe de serveurs. Ce groupe peut-être assimilé à un environnement (dev,qualif, prod....) et contient:

 * les group_vars: un all.yml et un fichier yaml par groupe qui en a besoin
 * les host_vars: à éviter
 * un fichier contenant les serveurs décrits dans l'inventory, qu'on nomme souvent "hosts"

### définition d'un serveur (ou host)

Le fichier hosts, comme son nom l'indique, définit tous les hosts de l'inventory. Un host est un serveur désigné par la variable *inventory_hostname*. l'*inventory_hostname* est soit un fqdn, soit une ip, soit un alias vers l'une des notions précédente. C'est avec cela qu'ansible va travailler sur les hosts. Il est recommandé de mettre un alias dans l'*inventory_hostname* et de renseigner la variable *ansible_host* avec la valeur du fqdn du host correspondant.
Il est intéressant de donner les mêmes *inventory_hostname* à des serveurs remplissant des fonctions similaires dans différents environnement afin d'uniformiser le déploiement et d'utiliser la notion d'*inventory_hostname* au mieux.

Info utile: il est tout à fait possible d'avoir 2 hosts qui pointent sur le même serveur pour uniformiser les environnements. Par exemple, si l'env de prod a un host "app1" pour app et "web1" web mais qu'en dev il n'y a qu'un serveur, on peut tout à fait créer 2 hosts "app1" et "dev1" qui poitenent sur le même serveur. 

### groupe de serveurs (ou group)

Un groupe est un ensemble de serveur qui va être désigné sous un nom commun et appelé avec ce nom. Cela permet de donner une fonction à un serveur, par exemple les serveurs de base de données seront regroupés sous le nom "bdd" ou "postgres". Un serveur peut appartenir à plusieurs groupes s'il remplit plusieurs fonction.

La notion de groupe est essentiel pour un inventory.

Le fichier hosts est écrit soit en INI soit en yaml, je trouve le ini très simple à lire quand c'est bien structuré. Le yaml un peu moins, surtout s'il y a de nombreux groupes ou hosts, mais c'est personnel... 
Un exemple est disponible dans */inventory/dev/hosts*

Infos utiles:

 * il existe 2 groupes "implicites": *all* (tous les hosts) et *ungrouped* (tous les hosts non groupés).
 * il existe un host particulier et implicite qui ne fait partie d'aucun groupe par défaut (ni même ceux mentionnés ci-dessus): *localhost*. Il désigne le serveur qui exécute le playbook, et donc qui a ansible d'installé. Il est assez utile dans certains cas, notamment pour déléguer des actions particulières comme envoyer un mail.


## Playbooks

Le dossier playbooks contient tous les playbooks (sauf les master-playbooks évidemment) ainsi que les group_vars et les host_vars qui correspondent. Il est possible de faire des sous-dossiers mais on perd les fichiers de variables par défaut.

## Roles

Ce dossier contient tous les rôles internes du projet.

# Infos et conseils

## Les variables

Les variables sont toutes les infos qu'ansible va utiliser pour exécuter les tâches de façon dynamique. Elles proviennent de 4 sources:

 * Du code

Tout ce qui n'est ni host ni playbook ou task est une variable. Elles sont définies à plusieurs endroits mais les plus courants sont:
  * les defaults du role.
  * les group_vars/host_vars des inventory
  * les group_vars/host_vars des playbooks

* De certaines tâches

le module *set_fact* permet de créer une variable. Il est également possible d'utiliser l'action *register* pour récupérer le retour d'une task sous forme de variable. Un exemple de register est disponible dans le role *app_service*.

* De la commande de lancement d'ansible.

Lors du lancement d'ansible, on peut passer certains paramètres à la commande. Ces paramètres sont utilisables sous forme de variables. Il est possible d'en passer d'autres qui se nomment *extra_vars* avec l'option -e (directement ou sous forme de fichier). Ces variables sont très utiles pour paramétrer un lancement d'ansible notamment avec une CI/CD.

* D'ansible lui-même

Par défaut, Ansible exécute une action nommé *gather_facts* sur tous les hosts de l'inventory. Cette action permet de récupérer de nombreuses info technique des hosts sur l'os, le cpu, la config réseau...

On peut les regrouper sous 3 formes:

*  les ansible_facts, qui regroupent les données de l'host courant. Les variables les plus utiles sont ansible_hostname et inventory_hostname.
* les groups, qui regroupent la liste des groupes ainsi que leur hosts sous forme de liste python. C'est assez utile pour savoir si un host fait partie d'un groupe ou non.
* les hostvars, qui regroupent TOUTES les infos de TOUS les hosts sous forme de tableau par host. C'est très utile pour retrouver les infos d'un host à partir d'un autre.

Il y a des exemples d'utilisation de hostvars dans le role postgresql.

### la précédence des variables

Sous cette notion barbare mais cependant essentiel d'ansible se cache un principe simple: la convention plutôt que la déclaration.

Par défaut, ansible va aller chercher ces variables dans plein d'endroits, la liste est donnée dans le lien en fin de cette partie. Si une variable est définie dans plusieurs de ces endroits, ansible va appliquer une règle de priorité et remplacer le contenu de la variable selon la prioriété établie.


## Bonnes pratiques

* éviter le module shell: shell n'est pas idempotent et ne prend pas forcément en compte les spécificité de l'os. Ansible utilise des modules afin d'éviter d'utiliser shell. Néanmoins il peut être utilisé s'il n'y a aucune autre alternative simple.
* Restez clair et cohérent dans l'écriture des playbooks.
* IDEMPOTENCE ! Un déploiement doit marcher dans tous les cas, et peut être relancé à l'infini sans conséquence.
* restez générique : éviter de créer des rôles spécifiques à une version d'un logiciel ou à un environnement.
* utiliser un vault pour cacher les données sensibles (hashi_corp vault ou ansible_vault)

# Test_Solutec



